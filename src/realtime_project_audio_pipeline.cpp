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
        track_bus_indices.reserve(project.tracks.size());
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
                                        runtime_graph.master_bus_id, setup_error)) {
            error = setup_error.code + ": " + setup_error.message;
            return false;
        }

        track_buffers_.swap(buffers);
        track_pointers_.swap(pointers);
        output_left_.swap(left);
        output_right_.swap(right);
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
