#pragma once

#include "hush/realtime_bus_engine.hpp"
#include "hush/realtime_graph_publisher.hpp"

#include <cstddef>
#include <memory>
#include <string>

namespace hush {

// Control-thread graph replacement host for the existing realtime bus engine.
// Replacement graphs are validated/configured off the callback and published as
// immutable topology snapshots. The audio thread holds a guard through processing
// and output copying; retired engines are destroyed only by control-thread publish/
// reclaim calls. Stop/join the audio reader before destroying this host.
class RealtimeBusEngineHost {
public:
    RealtimeBusEngineHost() = default;
    RealtimeBusEngineHost(const RealtimeBusEngineHost&) = delete;
    RealtimeBusEngineHost& operator=(const RealtimeBusEngineHost&) = delete;

    // Control thread only. Fails without replacing the active engine if graph/master
    // validation or processor setup fails. All replacement allocations happen here.
    bool replace_graph(const BusGraph& graph, std::size_t max_frames,
                       const std::string& master_bus_id, ProcessorSetupError& error);

    // Control thread only; use stable bus IDs to map to indices in the active topology.
    // Returns false for an invalid value/index, absent engine, or full bounded queue.
    [[nodiscard]] bool enqueue_fader_change(std::size_t bus_index, float gain) noexcept;

    // Audio thread only. No allocation, lock, logging, filesystem/network I/O, or GUI
    // work. Inputs are ordered like the graph's bus list. The selected master bus is
    // copied to the supplied planar outputs before the snapshot guard is released.
    [[nodiscard]] RealtimeProcessStatus process_to_stereo(
        const BusAudioInputView* inputs, std::size_t input_count, std::size_t frames,
        float* output_left, float* output_right) noexcept;

    [[nodiscard]] std::size_t bus_count_for_control() const noexcept;
    [[nodiscard]] std::size_t max_frames_for_control() const noexcept;
    void reclaim_retired() noexcept { publisher_.reclaim_retired(); }

private:
    struct Snapshot {
        // Processing mutates preallocated DSP state, while topology/lifetime remain guarded.
        mutable RealtimeBusEngine engine;
        std::size_t master_bus_index{0};
    };
    RealtimeGraphPublisher<Snapshot> publisher_;
};

} // namespace hush
