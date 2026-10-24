#include "hush/realtime_track_bus_router.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <utility>

namespace hush {

bool RealtimeTrackBusRouter::configure(
    const std::vector<std::size_t>& track_bus_indices,
    std::size_t bus_count, std::size_t max_frames) noexcept {
    if (configured_ || track_bus_indices.empty() || bus_count == 0 ||
        max_frames == 0 ||
        track_bus_indices.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) ||
        track_bus_indices.size() > kQueueCapacity) return false;
    for (const auto bus_index : track_bus_indices) {
        if (bus_index >= bus_count) return false;
    }

    try {
        auto new_assignments = track_bus_indices;
        std::vector<float> new_gains(track_bus_indices.size(), 1.0F);
        std::vector<float> new_pans(track_bus_indices.size(), 0.0F);
        std::vector<std::uint8_t> new_flags(track_bus_indices.size(), 0U);
        std::vector<std::vector<float>> new_left(bus_count);
        std::vector<std::vector<float>> new_right(bus_count);
        std::vector<BusAudioInputView> new_inputs(bus_count);
        for (std::size_t bus = 0; bus < bus_count; ++bus) {
            new_left[bus].assign(max_frames, 0.0F);
            new_right[bus].assign(max_frames, 0.0F);
            new_inputs[bus] = {new_left[bus].data(), new_right[bus].data()};
        }
        track_bus_indices_.swap(new_assignments);
        gains_.swap(new_gains);
        pans_.swap(new_pans);
        flags_.swap(new_flags);
        left_.swap(new_left);
        right_.swap(new_right);
        bus_inputs_.swap(new_inputs);
    } catch (...) {
        return false;
    }
    max_frames_ = max_frames;
    configured_ = true;
    return true;
}

bool RealtimeTrackBusRouter::enqueue_control(std::size_t track_index, float gain,
                                             float pan, bool mute, bool solo) noexcept {
    if (!configured_ || track_index >= track_bus_indices_.size() ||
        !std::isfinite(gain) || gain < 0.0F || gain > 2.0F ||
        !std::isfinite(pan) || pan < -1.0F || pan > 1.0F) return false;
    const auto flags = static_cast<std::uint8_t>((mute ? 1U : 0U) | (solo ? 2U : 0U));
    return commands_.try_push({static_cast<std::uint32_t>(track_index), gain, pan, flags});
}

bool RealtimeTrackBusRouter::process(const float* const* mono_tracks,
                                     std::size_t track_count,
                                     std::size_t frames) noexcept {
    if (!configured_ || track_count != track_bus_indices_.size() ||
        mono_tracks == nullptr || frames > max_frames_) return false;
    for (std::size_t track = 0; track < track_count; ++track) {
        if (mono_tracks[track] == nullptr) return false;
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

    for (std::size_t bus = 0; bus < bus_inputs_.size(); ++bus) {
        std::fill_n(left_[bus].data(), frames, 0.0F);
        std::fill_n(right_[bus].data(), frames, 0.0F);
    }
    bool any_solo = false;
    for (const auto flags : flags_) any_solo = any_solo || ((flags & 2U) != 0U);
    constexpr float kQuarterPi = 0.78539816339744830962F;
    for (std::size_t track = 0; track < track_count; ++track) {
        const auto flags = flags_[track];
        if ((flags & 1U) != 0U || (any_solo && (flags & 2U) == 0U)) continue;
        const auto bus = track_bus_indices_[track];
        const float angle = (pans_[track] + 1.0F) * kQuarterPi;
        const float left_gain = gains_[track] * std::cos(angle);
        const float right_gain = gains_[track] * std::sin(angle);
        for (std::size_t frame = 0; frame < frames; ++frame) {
            const float sample = mono_tracks[track][frame];
            left_[bus][frame] += sample * left_gain;
            right_[bus][frame] += sample * right_gain;
        }
    }
    return true;
}

} // namespace hush
