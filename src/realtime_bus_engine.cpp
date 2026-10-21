#include "hush/realtime_bus_engine.hpp"

#include <cmath>
#include <limits>
#include <utility>

namespace hush {

bool RealtimeBusEngine::configure(const BusGraph& graph, std::size_t max_frames,
                                  ProcessorSetupError& error) {
    if (configured_) {
        error = {"already_configured", "", "RealtimeBusEngine is configure-once; create a new instance to change topology"};
        return false;
    }
    if (graph.buses().size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        error = {"too_many_buses", "", "Bus count exceeds parameter queue index range"};
        return false;
    }

    // Allocate scratch on the control thread before exposing the engine to processing.
    std::vector<BusInputView> new_input_scratch(graph.buses().size());
    std::vector<float> new_fader_gains;
    std::vector<bool> new_muted;
    std::vector<bool> new_soloed;
    new_fader_gains.reserve(graph.buses().size());
    new_muted.reserve(graph.buses().size());
    new_soloed.reserve(graph.buses().size());
    for (const auto& bus : graph.buses()) {
        new_fader_gains.push_back(bus.gain);
        new_muted.push_back(bus.mute);
        new_soloed.push_back(bus.solo);
    }
    if (!processor_.configure(graph, max_frames, error)) return false;

    input_scratch_.swap(new_input_scratch);
    fader_gains_.swap(new_fader_gains);
    muted_.swap(new_muted);
    soloed_.swap(new_soloed);
    configured_ = true;
    error = {};
    return true;
}

bool RealtimeBusEngine::enqueue_fader_change(std::size_t bus_index, float gain) noexcept {
    if (!configured_ || bus_index >= fader_gains_.size() ||
        bus_index > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) ||
        !std::isfinite(gain) || gain < 0.0F || gain > 16.0F) {
        return false;
    }
    return parameter_queue_.try_push(
        {static_cast<std::uint32_t>(bus_index), gain, false, false, true, false, false});
}

bool RealtimeBusEngine::enqueue_bus_control(std::size_t bus_index, float gain,
                                           bool mute, bool solo) noexcept {
    if (!configured_ || bus_index >= fader_gains_.size() ||
        bus_index > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) ||
        !std::isfinite(gain) || gain < 0.0F || gain > 16.0F) {
        return false;
    }
    return parameter_queue_.try_push(
        {static_cast<std::uint32_t>(bus_index), gain, mute, solo, true, true, true});
}

RealtimeProcessStatus RealtimeBusEngine::process(const BusAudioInputView* inputs,
                                                 std::size_t input_count,
                                                 std::size_t frame_count) noexcept {
    if (!configured_) return RealtimeProcessStatus::NotConfigured;
    if (input_count != input_scratch_.size() ||
        (input_count != 0 && inputs == nullptr)) {
        return RealtimeProcessStatus::InvalidInputCount;
    }
    if (frame_count > processor_.max_frames()) {
        return RealtimeProcessStatus::FrameCountExceedsCapacity;
    }

    // Validate all caller-owned input fields before consuming queued changes.
    for (std::size_t i = 0; i < input_count; ++i) {
        if (inputs[i].left == nullptr || inputs[i].right == nullptr)
            return RealtimeProcessStatus::NullChannel;
    }

    BusControlChange change{};
    while (parameter_queue_.try_pop(change)) {
        const auto index = static_cast<std::size_t>(change.bus_index);
        // Values and indices are validated by the enqueue methods.
        if (index >= fader_gains_.size()) continue;
        if (change.update_gain) fader_gains_[index] = change.gain;
        if (change.update_mute) muted_[index] = change.mute;
        if (change.update_solo) soloed_[index] = change.solo;
    }

    bool any_solo = false;
    for (const bool solo : soloed_) any_solo = any_solo || solo;
    for (std::size_t i = 0; i < input_count; ++i) {
        const bool suppressed = muted_[i] || (any_solo && !soloed_[i]);
        const float effective_gain = suppressed ? 0.0F : fader_gains_[i];
        input_scratch_[i] = {inputs[i].left, inputs[i].right, effective_gain};
    }
    return processor_.process(input_scratch_.data(), input_scratch_.size(), frame_count);
}

} // namespace hush
