#include "hush/realtime_bus_engine_host.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace hush {

bool RealtimeBusEngineHost::replace_graph(const BusGraph& graph, std::size_t max_frames,
                                          const std::string& master_bus_id,
                                          ProcessorSetupError& error) {
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
        if (!replacement->engine.configure(graph, max_frames, error)) return false;
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

} // namespace hush
