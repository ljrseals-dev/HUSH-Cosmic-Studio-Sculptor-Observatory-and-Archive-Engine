#include "hush/realtime_project_audio_pipeline.hpp"

#include "hush/project_runtime_graph.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <utility>

namespace hush {

bool RealtimeProjectAudioPipeline::configure(const Project& project,
                                              std::size_t max_frames,
                                              std::string& error) {
    if (configured_ || max_frames == 0U || project.tracks.empty()) {
        error = "project_pipeline_invalid_state_capacity_or_empty_tracks";
        return false;
    }

    try {
        ProjectRuntimeGraph runtime_graph;
        RoutingError routing_error;
        if (!configure_project_runtime_graph(project, runtime_graph, routing_error)) {
            error = routing_error.code + ": " + routing_error.message;
            return false;
        }

        std::vector<std::size_t> track_bus_indices;
        std::vector<std::string> bus_ids;
        track_bus_indices.reserve(project.tracks.size());
        bus_ids.reserve(runtime_graph.graph.buses().size());
        for (const auto& bus : runtime_graph.graph.buses()) bus_ids.push_back(bus.id);
        for (const auto& track : project.tracks) {
            const std::string& bus_id = track.bus_id.empty()
                ? runtime_graph.input_bus_id : track.bus_id;
            std::size_t bus_index = runtime_graph.graph.buses().size();
            for (std::size_t i = 0; i < runtime_graph.graph.buses().size(); ++i) {
                if (runtime_graph.graph.buses()[i].id == bus_id) {
                    bus_index = i;
                    break;
                }
            }
            if (bus_index == runtime_graph.graph.buses().size()) {
                error = "track_bus_unknown: track '" + track.id +
                        "' references absent bus '" + bus_id + "'";
                return false;
            }
            track_bus_indices.push_back(bus_index);
        }

        std::vector<std::vector<float>> buffers(
            project.tracks.size(), std::vector<float>(max_frames, 0.0F));
        std::vector<const float*> pointers(project.tracks.size(), nullptr);
        for (std::size_t i = 0; i < buffers.size(); ++i) pointers[i] = buffers[i].data();
        std::vector<float> left(max_frames, 0.0F);
        std::vector<float> right(max_frames, 0.0F);
        auto bus_meters = std::make_unique<AtomicBusInputMeter[]>(runtime_graph.graph.buses().size());
        const auto bus_meter_count = runtime_graph.graph.buses().size();

        if (!track_router_.configure(std::move(track_bus_indices),
                                     runtime_graph.graph.buses().size(), max_frames)) {
            error = "track_bus_router_configuration_failed";
            return false;
        }

        // Prime controls before the device starts. Drain the bounded queue on the
        // control thread if a project has more tracks than one queue batch can hold.
        for (std::size_t i = 0; i < project.tracks.size(); ++i) {
            const auto& track = project.tracks[i];
            if (!track_router_.enqueue_track_control(i, track.gain, track.pan,
                                                    track.mute, track.solo)) {
                if (!track_router_.process(pointers.data(), pointers.size(), 0U) ||
                    !track_router_.enqueue_track_control(i, track.gain, track.pan,
                                                        track.mute, track.solo)) {
                    error = "track_control_queue_initialization_failed";
                    return false;
                }
            }
        }

        ProcessorSetupError setup_error;
        if (!engine_host_.replace_graph(runtime_graph.graph, max_frames,
                                        runtime_graph.master_bus_id, setup_error, project.inserts)) {
            error = setup_error.code + ": " + setup_error.message;
            return false;
        }

        track_buffers_.swap(buffers);
        track_pointers_.swap(pointers);
        output_left_.swap(left);
        output_right_.swap(right);
        bus_input_meters_ = std::move(bus_meters);
        bus_meter_count_ = bus_meter_count;
        bus_ids_.swap(bus_ids);
        input_bus_id_ = std::move(runtime_graph.input_bus_id);
        master_bus_id_ = std::move(runtime_graph.master_bus_id);
        max_frames_ = max_frames;
        configured_ = true;
        error.clear();
        return true;
    } catch (const std::exception&) {
        error = "project_pipeline_allocation_or_setup_failed";
        return false;
    } catch (...) {
        error = "project_pipeline_unknown_setup_failure";
        return false;
    }
}

bool RealtimeProjectAudioPipeline::enqueue_track_control(
    std::size_t track_index, float gain, float pan, bool mute, bool solo) noexcept {
    return configured_ &&
           track_router_.enqueue_track_control(track_index, gain, pan, mute, solo);
}

bool RealtimeProjectAudioPipeline::enqueue_bus_control(const std::string& bus_id, float gain,
                                                        bool mute, bool solo) noexcept {
    if (!configured_) return false;
    for (std::size_t i = 0; i < bus_ids_.size(); ++i) {
        if (bus_ids_[i] == bus_id)
            return engine_host_.enqueue_bus_control(i, gain, mute, solo);
    }
    return false;
}

bool RealtimeProjectAudioPipeline::process(const float* interleaved_input,
                                           float* interleaved_output,
                                           std::size_t frames) noexcept {
    if (interleaved_output == nullptr) return false;
    if (!configured_ || frames > max_frames_ ||
        (frames != 0U && interleaved_input == nullptr)) {
        std::fill_n(interleaved_output, frames * 2U, 0.0F);
        return false;
    }
    if (frames == 0U) return true;

    if (recording_ != nullptr) {
        // Capture the raw stereo input into the preallocated SPSC ring. Disk I/O is
        // performed only by RecordingSession's worker thread.
        (void)recording_->capture(std::span<const float>(interleaved_input, frames * 2U));
    }

    float input_peak = 0.0F;
    for (std::size_t i = 0; i < frames * 2U; ++i)
        input_peak = std::max(input_peak, std::abs(interleaved_input[i]));
    input_peak_.store(input_peak, std::memory_order_relaxed);

    for (auto& track : track_buffers_) std::fill_n(track.data(), frames, 0.0F);
    // The current ALSA adapter has two capture channels. They are exposed as two
    // mono source tracks; additional project tracks stay silent until a source is
    // explicitly connected by a recording/media playback implementation.
    if (!track_buffers_.empty()) {
        for (std::size_t frame = 0; frame < frames; ++frame)
            track_buffers_[0][frame] = interleaved_input[frame * 2U];
    }
    if (track_buffers_.size() > 1U) {
        for (std::size_t frame = 0; frame < frames; ++frame)
            track_buffers_[1][frame] = interleaved_input[frame * 2U + 1U];
    }

    if (!track_router_.process(track_pointers_.data(), track_pointers_.size(), frames)) {
        std::fill_n(interleaved_output, frames * 2U, 0.0F);
        return false;
    }

    // Publish lock-free block-level measurements for every routed bus input. This
    // deliberately measures the signal entering the bus graph, not post-fader or
    // post-insert output; those require a separate engine-stage meter contract.
    const auto* bus_inputs = track_router_.bus_inputs();
    for (std::size_t bus = 0; bus < bus_meter_count_; ++bus) {
        float peak_left = 0.0F;
        float peak_right = 0.0F;
        double sum_left = 0.0;
        double sum_right = 0.0;
        for (std::size_t frame = 0; frame < frames; ++frame) {
            const float left = std::isfinite(bus_inputs[bus].left[frame]) ? bus_inputs[bus].left[frame] : 0.0F;
            const float right = std::isfinite(bus_inputs[bus].right[frame]) ? bus_inputs[bus].right[frame] : 0.0F;
            peak_left = std::max(peak_left, std::abs(left));
            peak_right = std::max(peak_right, std::abs(right));
            sum_left += static_cast<double>(left) * static_cast<double>(left);
            sum_right += static_cast<double>(right) * static_cast<double>(right);
        }
        const float rms_left = static_cast<float>(std::sqrt(sum_left / static_cast<double>(frames)));
        const float rms_right = static_cast<float>(std::sqrt(sum_right / static_cast<double>(frames)));
        bus_input_meters_[bus].peak_left.store(peak_left, std::memory_order_relaxed);
        bus_input_meters_[bus].peak_right.store(peak_right, std::memory_order_relaxed);
        bus_input_meters_[bus].rms_left.store(rms_left, std::memory_order_relaxed);
        bus_input_meters_[bus].rms_right.store(rms_right, std::memory_order_relaxed);
    }

    const auto status = engine_host_.process_to_stereo(
        track_router_.bus_inputs(), track_router_.bus_count(), frames,
        output_left_.data(), output_right_.data());
    if (status != RealtimeProcessStatus::Ok) {
        std::fill_n(interleaved_output, frames * 2U, 0.0F);
        return false;
    }

    float peak = 0.0F;
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const float left = std::isfinite(output_left_[frame]) ? output_left_[frame] : 0.0F;
        const float right = std::isfinite(output_right_[frame]) ? output_right_[frame] : 0.0F;
        interleaved_output[frame * 2U] = left;
        interleaved_output[frame * 2U + 1U] = right;
        peak = std::max(peak, std::abs(left));
        peak = std::max(peak, std::abs(right));
    }
    output_peak_.store(peak, std::memory_order_relaxed);
    return true;
}

BusInputMeterSnapshot RealtimeProjectAudioPipeline::bus_input_meter(std::size_t bus_index) const noexcept {
    if (bus_index >= bus_meter_count_ || bus_input_meters_ == nullptr) return {};
    const auto& meter = bus_input_meters_[bus_index];
    return {meter.peak_left.load(std::memory_order_relaxed),
            meter.peak_right.load(std::memory_order_relaxed),
            meter.rms_left.load(std::memory_order_relaxed),
            meter.rms_right.load(std::memory_order_relaxed)};
}

void RealtimeProjectAudioPipeline::alsa_callback(const float* input, float* output,
                                                  std::size_t frames,
                                                  void* user_data) noexcept {
    auto* pipeline = static_cast<RealtimeProjectAudioPipeline*>(user_data);
    if (pipeline == nullptr) {
        if (output != nullptr) std::fill_n(output, frames * 2U, 0.0F);
        return;
    }
    (void)pipeline->process(input, output, frames);
}

} // namespace hush
