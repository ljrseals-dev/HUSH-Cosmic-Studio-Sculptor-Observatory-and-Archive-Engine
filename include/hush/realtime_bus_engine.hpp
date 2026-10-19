#pragma once

#include "hush/preallocated_bus_processor.hpp"
#include "hush/spsc_parameter_queue.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace hush {

// Control-to-audio-thread fader update. bus_index follows BusGraph::buses() order.
struct BusAudioInputView {
    const float* left{};
    const float* right{};
};

struct BusFaderChange {
    std::uint32_t bus_index{};
    float gain{1.0F};
};

// Integrates the fixed-capacity parameter queue with the preallocated bus processor.
// Configure once on the control thread before publishing this object to the audio thread.
// After configure(), enqueue_fader_change() has one producer and process() has one
// consumer. Do not configure, destroy, or query mutable processor state concurrently
// with process(). No allocations, locks, logging, or I/O occur in process().
class RealtimeBusEngine {
public:
    static constexpr std::size_t kParameterQueueCapacity = 1024;

    bool configure(const BusGraph& graph, std::size_t max_frames,
                   ProcessorSetupError& error);

    // Single control/UI producer. Returns false for invalid values or a full queue.
    [[nodiscard]] bool enqueue_fader_change(std::size_t bus_index, float gain) noexcept;

    // Single audio-thread consumer. Inputs must be ordered like graph.buses().
    [[nodiscard]] RealtimeProcessStatus process(const BusAudioInputView* inputs,
                                                std::size_t input_count,
                                                std::size_t frame_count) noexcept;

    [[nodiscard]] std::size_t bus_count() const noexcept { return processor_.bus_count(); }
    [[nodiscard]] std::size_t max_frames() const noexcept { return processor_.max_frames(); }
    [[nodiscard]] StereoConstView pre_fader(std::size_t bus_index,
                                            std::size_t frames) const noexcept {
        return processor_.pre_fader(bus_index, frames);
    }
    [[nodiscard]] StereoConstView post_fader(std::size_t bus_index,
                                             std::size_t frames) const noexcept {
        return processor_.post_fader(bus_index, frames);
    }
    [[nodiscard]] bool parameter_atomics_are_lock_free() const noexcept {
        return parameter_queue_.atomics_are_lock_free();
    }

private:
    PreallocatedBusProcessor processor_;
    SpscParameterQueue<BusFaderChange, kParameterQueueCapacity> parameter_queue_;
    std::vector<BusInputView> input_scratch_;
    std::vector<float> fader_gains_;
    bool configured_{false};
};

} // namespace hush
