#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hush {

// MPE channels are zero-based (0..15). A note owns its expression data so
// simultaneous notes can carry independent pitch, pressure, and timbre curves.
enum class MpeDimension : std::uint8_t {
    PitchSemitones = 0,
    Pressure = 1,
    Timbre = 2
};

struct MpeExpressionPoint {
    std::uint64_t offset_frames{0};
    MpeDimension dimension{MpeDimension::Pressure};
    float value{0.0F};
    bool operator==(const MpeExpressionPoint&) const = default;
};

struct MpeNote {
    std::string id;
    std::uint8_t channel{0};
    std::uint8_t key{60};
    std::uint8_t velocity{100};
    std::uint64_t start_frame{0};
    std::uint64_t duration_frames{1};
    // Sorted by (offset_frames, dimension); at most one value per dimension
    // at a given frame. Offsets are relative to this note, not the session.
    std::vector<MpeExpressionPoint> expression;
    bool operator==(const MpeNote&) const = default;
};

struct MpeValidationError {
    const char* code{nullptr};
    std::string note_id;
    const char* message{nullptr};
};

// Validation is a control-thread operation. It checks bounds, overflow,
// deterministic ordering, duplicate points, and finite expression values.
[[nodiscard]] bool validate_mpe_note(const MpeNote& note,
                                     MpeValidationError& error);

// Returns a linearly interpolated value for one dimension. Values before the
// first point and after the last point hold the nearest endpoint. If a note
// has no points for the dimension, fallback_value is returned.
[[nodiscard]] float evaluate_mpe_expression(const MpeNote& note,
                                            MpeDimension dimension,
                                            std::uint64_t offset_frames,
                                            float fallback_value) noexcept;

// Stable channel index for UI color assignment; this does not prescribe a
// palette, so frontends can apply their current theme without changing data.
[[nodiscard]] constexpr std::uint8_t mpe_channel_color_index(
    std::uint8_t channel) noexcept {
    return channel < 16U ? channel : 0U;
}

} // namespace hush
