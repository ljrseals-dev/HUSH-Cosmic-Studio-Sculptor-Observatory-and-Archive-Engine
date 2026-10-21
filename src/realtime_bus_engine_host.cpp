#include "hush/realtime_bus_engine_host.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <memory>
#include <utility>

namespace hush {

bool RealtimeBusEngineHost::replace_graph(const BusGraph& graph, std::size_t max_frames,
                                          const std::string& master_bus_id,
                                          ProcessorSetupError& error,
                                          const std::vector<BusInsert>& inserts) {
    if (!graph.valid()) {
        error = {"bus_graph_invalid", "", "Replacement graph must be validated before publication"};
        return false;
    }

    std::size_t master_index = graph.buses().size();
    for (std::size_t i = 0; i < graph.buses().size(); ++i) {
        if (graph.buses()[i].id == master_bus_id) {
            master_index = i;
            break;
        }
    }
    if (master_index == graph.buses().size()) {
        error = {"master_bus_not_found", master_bus_id, "Master bus ID is absent from replacement graph"};
        return false;
    }

    try {
        auto replacement = std::make_unique<Snapshot>();
        if (!replacement->engine.configure(graph, max_frames, error, inserts)) return false;
        replacement->meters = std::make_unique<AtomicBusOutputMeter[]>(graph.buses().size());
        replacement->meter_count = graph.buses().size();
        replacement->master_bus_index = master_index;
        // publish() and any retired-snapshot destruction happen on this control thread.
        publisher_.publish(std::move(replacement));
    } catch (const std::exception&) {
        error = {"graph_publication_failed", master_bus_id,
                 "Could not allocate or publish replacement graph; active graph was retained"};
        return false;
    }
    error = {};
    return true;
}

bool RealtimeBusEngineHost::enqueue_fader_change(std::size_t bus_index, float gain) noexcept {
    auto* snapshot = publisher_.active_for_control();
    return snapshot != nullptr && snapshot->engine.enqueue_fader_change(bus_index, gain);
}

bool RealtimeBusEngineHost::enqueue_bus_control(std::size_t bus_index, float gain,
                                                bool mute, bool solo) noexcept {
    auto* snapshot = publisher_.active_for_control();
    return snapshot != nullptr &&
           snapshot->engine.enqueue_bus_control(bus_index, gain, mute, solo);
}

RealtimeProcessStatus RealtimeBusEngineHost::process_to_stereo(
    const BusAudioInputView* inputs, std::size_t input_count, std::size_t frames,
    float* output_left, float* output_right) noexcept {
    if (output_left == nullptr || output_right == nullptr) return RealtimeProcessStatus::NullChannel;

    auto guard = publisher_.acquire_for_audio();
    if (!guard) {
        std::fill_n(output_left, frames, 0.0F);
        std::fill_n(output_right, frames, 0.0F);
        return RealtimeProcessStatus::NotConfigured;
    }

    const auto status = guard->engine.process(inputs, input_count, frames);
    if (status != RealtimeProcessStatus::Ok) {
        std::fill_n(output_left, frames, 0.0F);
        std::fill_n(output_right, frames, 0.0F);
        return status;
    }

    for (std::size_t bus = 0; bus < guard->meter_count; ++bus) {
        const auto signal = guard->engine.post_fader(bus, frames);
        float peak_left = 0.0F, peak_right = 0.0F;
        double sum_left = 0.0, sum_right = 0.0;
        if (signal.left != nullptr && signal.right != nullptr) {
            for (std::size_t frame = 0; frame < frames; ++frame) {
                const float left = std::isfinite(signal.left[frame]) ? signal.left[frame] : 0.0F;
                const float right = std::isfinite(signal.right[frame]) ? signal.right[frame] : 0.0F;
                peak_left = std::max(peak_left, std::abs(left));
                peak_right = std::max(peak_right, std::abs(right));
                sum_left += static_cast<double>(left) * static_cast<double>(left);
                sum_right += static_cast<double>(right) * static_cast<double>(right);
            }
        }
        const float rms_left = frames == 0U ? 0.0F :
            static_cast<float>(std::sqrt(sum_left / static_cast<double>(frames)));
        const float rms_right = frames == 0U ? 0.0F :
            static_cast<float>(std::sqrt(sum_right / static_cast<double>(frames)));
        guard->meters[bus].peak_left.store(peak_left, std::memory_order_relaxed);
        guard->meters[bus].peak_right.store(peak_right, std::memory_order_relaxed);
        guard->meters[bus].rms_left.store(rms_left, std::memory_order_relaxed);
        guard->meters[bus].rms_right.store(rms_right, std::memory_order_relaxed);
    }

    const auto master = guard->engine.post_fader(guard->master_bus_index, frames);
    if (master.left == nullptr || master.right == nullptr) {
        std::fill_n(output_left, frames, 0.0F);
        std::fill_n(output_right, frames, 0.0F);
        return RealtimeProcessStatus::NotConfigured;
    }
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const float left = master.left[frame];
        const float right = master.right[frame];
        output_left[frame] = std::isfinite(left) ? left : 0.0F;
        output_right[frame] = std::isfinite(right) ? right : 0.0F;
    }
    return RealtimeProcessStatus::Ok;
}

std::size_t RealtimeBusEngineHost::bus_count_for_control() const noexcept {
    auto* snapshot = publisher_.active_for_control();
    return snapshot == nullptr ? 0U : snapshot->engine.bus_count();
}

BusOutputMeterSnapshot RealtimeBusEngineHost::bus_output_meter(std::size_t bus_index) const noexcept {
    const auto* snapshot = publisher_.active_for_control();
    if (snapshot == nullptr || bus_index >= snapshot->meter_count) return {};
    const auto& meter = snapshot->meters[bus_index];
    return {
        meter.peak_left.load(std::memory_order_relaxed),
        meter.peak_right.load(std::memory_order_relaxed),
        meter.rms_left.load(std::memory_order_relaxed),
        meter.rms_right.load(std::memory_order_relaxed)
    };
}

std::size_t RealtimeBusEngineHost::max_frames_for_control() const noexcept {
    auto* snapshot = publisher_.active_for_control();
    return snapshot == nullptr ? 0U : snapshot->engine.max_frames();
}

} // namespace hush
