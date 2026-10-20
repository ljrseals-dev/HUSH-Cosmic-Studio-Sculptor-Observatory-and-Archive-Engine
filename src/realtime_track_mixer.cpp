#include "hush/realtime_track_mixer.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <new>
#include <vector>

namespace hush {

float RealtimeTrackMixer::peak_left() const noexcept {
    return std::bit_cast<float>(peak_left_bits_.load(std::memory_order_relaxed));
}
float RealtimeTrackMixer::peak_right() const noexcept {
    return std::bit_cast<float>(peak_right_bits_.load(std::memory_order_relaxed));
}
float RealtimeTrackMixer::rms_left() const noexcept {
    return std::bit_cast<float>(rms_left_bits_.load(std::memory_order_relaxed));
}
float RealtimeTrackMixer::rms_right() const noexcept {
    return std::bit_cast<float>(rms_right_bits_.load(std::memory_order_relaxed));
}

bool RealtimeTrackMixer::configure(std::size_t track_count, std::size_t max_frames) noexcept {
    if (configured_ || track_count == 0 ||
        track_count > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) ||
        max_frames == 0) return false;
    try {
        gains_.assign(track_count, 1.0F);
        pans_.assign(track_count, 0.0F);
        flags_.assign(track_count, 0U);
    } catch (const std::bad_alloc&) {
        gains_.clear(); pans_.clear(); flags_.clear();
        return false;
    } catch (...) {
        gains_.clear(); pans_.clear(); flags_.clear();
        return false;
    }
    max_frames_ = max_frames;
    configured_ = true;
    return true;
}

bool RealtimeTrackMixer::enqueue_control(std::size_t track_index, float gain,
                                         float pan, bool mute, bool solo) noexcept {
    if (!configured_ || track_index >= gains_.size() ||
        track_index > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) ||
        !std::isfinite(gain) || gain < 0.0F || gain > 2.0F ||
        !std::isfinite(pan) || pan < -1.0F || pan > 1.0F) return false;
    const auto flags = static_cast<std::uint8_t>((mute ? 1U : 0U) | (solo ? 2U : 0U));
    return commands_.try_push({static_cast<std::uint32_t>(track_index), gain, pan, flags});
}

bool RealtimeTrackMixer::process(const float* const* mono_tracks,
                                 std::size_t track_count,
                                 float* output_left, float* output_right,
                                 std::size_t frames) noexcept {
    if (!configured_ || track_count != gains_.size() ||
        (track_count != 0 && mono_tracks == nullptr) ||
        output_left == nullptr || output_right == nullptr || frames > max_frames_) return false;
    for (std::size_t i = 0; i < track_count; ++i) {
        if (mono_tracks[i] == nullptr) return false;
    }

    TrackControlCommand command{};
    while (commands_.try_pop(command)) {
        const auto index = static_cast<std::size_t>(command.track_index);
        if (index < gains_.size()) {
            gains_[index] = command.gain;
            pans_[index] = command.pan;
            flags_[index] = command.flags;
        }
    }

    bool any_solo = false;
    for (const auto flags : flags_) any_solo = any_solo || ((flags & 2U) != 0U);
    std::fill_n(output_left, frames, 0.0F);
    std::fill_n(output_right, frames, 0.0F);
    constexpr float kQuarterPi = 0.78539816339744830962F;
    for (std::size_t track = 0; track < track_count; ++track) {
        const auto flags = flags_[track];
        if ((flags & 1U) != 0U || (any_solo && (flags & 2U) == 0U)) continue;
        const float angle = (pans_[track] + 1.0F) * kQuarterPi;
        const float left_gain = gains_[track] * std::cos(angle);
        const float right_gain = gains_[track] * std::sin(angle);
        for (std::size_t frame = 0; frame < frames; ++frame) {
            const float sample = mono_tracks[track][frame];
            output_left[frame] += sample * left_gain;
            output_right[frame] += sample * right_gain;
        }
    }
    float peak_l = 0.0F;
    float peak_r = 0.0F;
    double sum_l = 0.0;
    double sum_r = 0.0;
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const float left = output_left[frame];
        const float right = output_right[frame];
        peak_l = std::max(peak_l, std::abs(left));
        peak_r = std::max(peak_r, std::abs(right));
        sum_l += static_cast<double>(left) * static_cast<double>(left);
        sum_r += static_cast<double>(right) * static_cast<double>(right);
    }
    const float rms_l = frames == 0 ? 0.0F : static_cast<float>(std::sqrt(sum_l / static_cast<double>(frames)));
    const float rms_r = frames == 0 ? 0.0F : static_cast<float>(std::sqrt(sum_r / static_cast<double>(frames)));
    peak_left_bits_.store(std::bit_cast<std::uint32_t>(peak_l), std::memory_order_relaxed);
    peak_right_bits_.store(std::bit_cast<std::uint32_t>(peak_r), std::memory_order_relaxed);
    rms_left_bits_.store(std::bit_cast<std::uint32_t>(rms_l), std::memory_order_relaxed);
    rms_right_bits_.store(std::bit_cast<std::uint32_t>(rms_r), std::memory_order_relaxed);
    return true;
}

} // namespace hush
