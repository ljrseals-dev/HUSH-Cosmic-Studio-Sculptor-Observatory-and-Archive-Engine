#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace hush {

// MIDI channels are zero-based (0..15); controller and value are 7-bit (0..127).
struct MidiCcEvent {
    std::uint8_t channel{};
    std::uint8_t controller{};
    std::uint8_t value{};
};

// Stable numeric IDs keep MIDI mapping independent of UI labels and project strings.
// The owner must reserve 0 as invalid.
using ParameterId = std::uint32_t;

struct MidiLearnBinding {
    std::uint8_t channel{};
    std::uint8_t controller{};
    ParameterId parameter_id{};
    float minimum{};
    float maximum{1.0F};
    bool inverted{false};
};

struct MappedParameterValue {
    ParameterId parameter_id{};
    float value{};
};

// Fixed-capacity, allocation-free CC mapping. Configure on the control thread
// before publishing the map to the MIDI/audio path, then treat it as immutable.
// This class does not open MIDI devices, perform I/O, or apply parameters itself.
class MidiLearnMap {
public:
    static constexpr std::size_t kMaxBindings = 128;

    // Replaces the complete map atomically only in the logical/configuration sense:
    // call before sharing with realtime readers; do not mutate while readers run.
    [[nodiscard]] bool set_bindings(const MidiLearnBinding* bindings,
                                    std::size_t count) noexcept;

    [[nodiscard]] bool map_cc(const MidiCcEvent& event,
                              MappedParameterValue& output) const noexcept;

    [[nodiscard]] std::size_t size() const noexcept { return count_; }

private:
    std::array<MidiLearnBinding, kMaxBindings> bindings_{};
    std::size_t count_{0};
};

} // namespace hush
