#pragma once

#include "hush/realtime_bus_engine.hpp"
#include "hush/realtime_graph_publisher.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>

namespace hush {

// Control-thread publisher for complete, preconfigured bus-engine snapshots.
// Build/configure replacements before publish_graph(); process() is the single
// audio-reader entry point and holds the lifetime guard for the full callback.
// Control operations (publish_graph/enqueue_fader_change/reclaim_retired) must
// be serialized on one control thread. Do not retain pointers or views into an
// engine after process() returns.
class PublishedRealtimeBusEngine {
public:
    bool publish_graph(const BusGraph& graph, std::size_t max_frames,
                       ProcessorSetupError& error) {
        auto replacement = std::make_unique<RealtimeBusEngine>();
        if (!replacement->configure(graph, max_frames, error)) return false;
        publisher_.publish(std::move(replacement));
        return true;
    }

    [[nodiscard]] bool enqueue_fader_change(std::size_t bus_index,
                                            float gain) noexcept {
        auto* active = publisher_.active_for_control();
        return active && active->enqueue_fader_change(bus_index, gain);
    }

    [[nodiscard]] RealtimeProcessStatus process(const BusAudioInputView* inputs,
                                                std::size_t input_count,
                                                std::size_t frame_count) noexcept {
        auto guard = publisher_.acquire_for_audio();
        if (!guard) return RealtimeProcessStatus::NotConfigured;
        return guard->process(inputs, input_count, frame_count);
    }

    // Process and copy the selected master bus while the lifetime guard is held.
    // This prevents a concurrent control-thread publication/reclamation from
    // invalidating the post-fader view before it has been copied to the device.
    [[nodiscard]] RealtimeProcessStatus process_to_master(
        const BusAudioInputView* inputs, std::size_t input_count,
        std::size_t frame_count, std::size_t master_bus_index,
        float* output_left, float* output_right) noexcept {
        if (output_left == nullptr || output_right == nullptr)
            return RealtimeProcessStatus::NullChannel;
        auto guard = publisher_.acquire_for_audio();
        if (!guard) return RealtimeProcessStatus::NotConfigured;
        const auto status = guard->process(inputs, input_count, frame_count);
        if (status != RealtimeProcessStatus::Ok) return status;
        const auto master = guard->post_fader(master_bus_index, frame_count);
        if (master.left == nullptr || master.right == nullptr)
            return RealtimeProcessStatus::InvalidInputCount;
        for (std::size_t frame = 0; frame < frame_count; ++frame) {
            const float left = master.left[frame];
            const float right = master.right[frame];
            output_left[frame] = std::isfinite(left) ? left : 0.0F;
            output_right[frame] = std::isfinite(right) ? right : 0.0F;
        }
        return RealtimeProcessStatus::Ok;
    }

    void reclaim_retired() noexcept { publisher_.reclaim_retired(); }

    [[nodiscard]] std::size_t retired_count_for_control() const noexcept {
        return publisher_.retired_count_for_control();
    }

private:
    RealtimeGraphPublisher<RealtimeBusEngine> publisher_;
};

} // namespace hush
