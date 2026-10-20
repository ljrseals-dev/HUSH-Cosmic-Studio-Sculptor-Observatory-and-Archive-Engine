#include "hush/realtime_track_mixer.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <vector>

namespace hush {

bool RealtimeTrackMixer::configure(std::size_t track_count, std::size_t max_frames) noexcept {
    if (configured_ || track_count == 0 || track_count > kMaxTracks ||
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
    active_track_count_ = track_count;
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

bool RealtimeTrackMixer::enqueue_active_track_count(std::size_t active_count) noexcept {
    if (!configured_ || active_count > gains_.size() ||
        active_count > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) return false;
    TrackControlCommand command{};
    command.type = TrackCommandType::SetTrackCount;
    command.active_track_count = static_cast<std::uint32_t>(active_count);
    return commands_.try_push(command);
}

bool RealtimeTrackMixer::enqueue_track_state_and_count(std::size_t track_index, float gain,
                                                        float pan, bool mute, bool solo,
                                                        std::size_t active_count) noexcept {
    if (!configured_ || track_index >= gains_.size() || active_count > gains_.size() ||
        active_count > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) ||
        track_index > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) ||
        !std::isfinite(gain) || gain < 0.0F || gain > 2.0F ||
        !std::isfinite(pan) || pan < -1.0F || pan > 1.0F) return false;
    TrackControlCommand command{};
    command.track_index = static_cast<std::uint32_t>(track_index);
    command.gain = gain;
    command.pan = pan;
    command.flags = static_cast<std::uint8_t>((mute ? 1U : 0U) | (solo ? 2U : 0U));
    command.type = TrackCommandType::ControlsAndTrackCount;
    command.active_track_count = static_cast<std::uint32_t>(active_count);
    return commands_.try_push(command);
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
        if (command.type == TrackCommandType::SetTrackCount) {
            active_track_count_ = static_cast<std::size_t>(command.active_track_count);
            continue;
        }
        const auto index = static_cast<std::size_t>(command.track_index);
        if (index < gains_.size()) {
            gains_[index] = command.gain;
            pans_[index] = command.pan;
            flags_[index] = command.flags;
        }
        if (command.type == TrackCommandType::ControlsAndTrackCount)
            active_track_count_ = static_cast<std::size_t>(command.active_track_count);
    }

    bool any_solo = false;
    for (std::size_t track = 0; track < active_track_count_; ++track)
        any_solo = any_solo || ((flags_[track] & 2U) != 0U);
    std::fill_n(output_left, frames, 0.0F);
    std::fill_n(output_right, frames, 0.0F);
    constexpr float kQuarterPi = 0.78539816339744830962F;
    for (std::size_t track = 0; track < active_track_count_; ++track) {
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
    return true;
}

} // namespace hush
