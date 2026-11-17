#include "hush/mpe_expression.hpp"

#include <cmath>
#include <limits>

namespace hush {
namespace {
bool valid_dimension(MpeDimension dimension) noexcept {
    return dimension == MpeDimension::PitchSemitones ||
           dimension == MpeDimension::Pressure ||
           dimension == MpeDimension::Timbre;
}

bool valid_value(MpeDimension dimension, float value) noexcept {
    if (!std::isfinite(value)) return false;
    if (dimension == MpeDimension::PitchSemitones)
        return value >= -96.0F && value <= 96.0F;
    return value >= 0.0F && value <= 1.0F;
}

bool less_point(const MpeExpressionPoint& a,
                const MpeExpressionPoint& b) noexcept {
    if (a.offset_frames != b.offset_frames)
        return a.offset_frames < b.offset_frames;
    return static_cast<std::uint8_t>(a.dimension) <
           static_cast<std::uint8_t>(b.dimension);
}
} // namespace

bool validate_mpe_note(const MpeNote& note,
                       MpeValidationError& error) noexcept {
    error = {};
    const auto fail = [&error, &note](const char* code,
                                      const char* message) noexcept {
        error.code = code;
        error.note_id = note.id;
        error.message = message;
        return false;
    };

    if (note.id.empty()) return fail("mpe_note_id_empty", "MPE note ID must not be empty");
    if (note.channel > 15U) return fail("mpe_channel_out_of_range", "MPE channel must be 0..15");
    if (note.key > 127U) return fail("mpe_key_out_of_range", "MIDI key must be 0..127");
    if (note.velocity == 0U || note.velocity > 127U)
        return fail("mpe_velocity_out_of_range", "MIDI note velocity must be 1..127");
    if (note.duration_frames == 0U)
        return fail("mpe_duration_zero", "MPE note duration must be greater than zero");
    if (note.start_frame > std::numeric_limits<std::uint64_t>::max() -
                                note.duration_frames)
        return fail("mpe_timeline_overflow", "MPE note end frame overflows uint64");

    const MpeExpressionPoint* previous = nullptr;
    for (const auto& point : note.expression) {
        if (!valid_dimension(point.dimension))
            return fail("mpe_dimension_invalid", "Unknown MPE expression dimension");
        if (point.offset_frames > note.duration_frames)
            return fail("mpe_point_outside_note", "Expression point lies beyond note duration");
        if (!valid_value(point.dimension, point.value))
            return fail("mpe_value_out_of_range", "MPE expression value is non-finite or outside its range");
        if (previous && !less_point(*previous, point))
            return fail("mpe_points_not_strictly_ordered",
                        "Expression points must be sorted with no duplicate dimension/frame pair");
        previous = &point;
    }
    return true;
}

float evaluate_mpe_expression(const MpeNote& note,
                              MpeDimension dimension,
                              std::uint64_t offset_frames,
                              float fallback_value) noexcept {
    const MpeExpressionPoint* first = nullptr;
    const MpeExpressionPoint* previous = nullptr;
    for (const auto& point : note.expression) {
        if (point.dimension != dimension) continue;
        if (!first) first = &point;
        if (point.offset_frames >= offset_frames) {
            if (!previous) return point.value;
            const auto span = point.offset_frames - previous->offset_frames;
            if (span == 0U) return point.value;
            const float amount = static_cast<float>(offset_frames - previous->offset_frames) /
                                 static_cast<float>(span);
            return previous->value + (point.value - previous->value) * amount;
        }
        previous = &point;
    }
    if (previous) return previous->value;
    return first ? first->value : fallback_value;
}
} // namespace hush
