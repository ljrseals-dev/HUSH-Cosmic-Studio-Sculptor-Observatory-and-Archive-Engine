#include "hush/mpe_expression.hpp"

#include <cassert>
#include <cmath>
#include <limits>

int main() {
    using namespace hush;
    MpeNote note{"note-1", 2, 64, 110, 1000, 1000,
        {{0, MpeDimension::PitchSemitones, 0.0F},
         {0, MpeDimension::Pressure, 0.2F},
         {500, MpeDimension::PitchSemitones, 12.0F},
         {500, MpeDimension::Pressure, 0.8F},
         {1000, MpeDimension::Timbre, 1.0F}}};
    MpeValidationError error;
    assert(validate_mpe_note(note, error));
    assert(error.code == nullptr);
    assert(std::abs(evaluate_mpe_expression(note, MpeDimension::PitchSemitones, 250, -1.0F) - 6.0F) < 1.0e-6F);
    assert(std::abs(evaluate_mpe_expression(note, MpeDimension::Pressure, 250, 0.0F) - 0.5F) < 1.0e-6F);
    assert(evaluate_mpe_expression(note, MpeDimension::Timbre, 0, 0.4F) == 1.0F);
    assert(evaluate_mpe_expression(note, MpeDimension::Timbre, 1000, 0.4F) == 1.0F);
    assert(evaluate_mpe_expression(note, MpeDimension::Pressure, 900, 0.0F) == 0.8F);
    assert(mpe_channel_color_index(15) == 15);
    assert(mpe_channel_color_index(16) == 0);

    auto invalid = note;
    invalid.channel = 16;
    assert(!validate_mpe_note(invalid, error) && std::string(error.code) == "mpe_channel_out_of_range");
    invalid = note; invalid.duration_frames = 0;
    assert(!validate_mpe_note(invalid, error) && std::string(error.code) == "mpe_duration_zero");
    invalid = note; invalid.expression[2].value = std::numeric_limits<float>::quiet_NaN();
    assert(!validate_mpe_note(invalid, error) && std::string(error.code) == "mpe_value_out_of_range");
    invalid = note; invalid.expression[2].offset_frames = 0;
    assert(!validate_mpe_note(invalid, error) && std::string(error.code) == "mpe_points_not_strictly_ordered");
    invalid = note; invalid.expression[0].value = 97.0F;
    assert(!validate_mpe_note(invalid, error) && std::string(error.code) == "mpe_value_out_of_range");
    invalid = note; invalid.start_frame = std::numeric_limits<std::uint64_t>::max();
    assert(!validate_mpe_note(invalid, error) && std::string(error.code) == "mpe_timeline_overflow");
    return 0;
}
