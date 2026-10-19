#pragma once

#include "hush/realtime_bus_engine.hpp"
#include "hush/realtime_graph_publisher.hpp"

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

    void reclaim_retired() noexcept { publisher_.reclaim_retired(); }

    [[nodiscard]] std::size_t retired_count_for_control() const noexcept {
        return publisher_.retired_count_for_control();
    }

private:
    RealtimeGraphPublisher<RealtimeBusEngine> publisher_;
};

} // namespace hush
