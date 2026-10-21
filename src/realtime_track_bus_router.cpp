#include "hush/realtime_track_bus_router.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace hush {

bool RealtimeTrackBusRouter::configure(std::vector<std::size_t> track_bus_indices,
                                       std::size_t bus_count,
                                       std::size_t max_frames) noexcept {
    if (configured_ || track_bus_indices.empty() || bus_count == 0 || max_frames == 0 ||
        track_bus_indices.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
        return false;
    for (const auto bus_index : track_bus_indices) {
        if (bus_index >= bus_count) return false;
    }

    try {
        std::vector<float> gains(track_bus_indices.size(), 1.0F);
        std::vector<float> pans(track_bus_indices.size(), 0.0F);
        std::vector<std::uint8_t> flags(track_bus_indices.size(), 0U);
        std::vector<std::vector<float>> left(bus_count, std::vector<float>(max_frames, 0.0F));
        std::vector<std::vector<float>> right(bus_count, std::vector<float>(max_frames, 0.0F));
        std::vector<BusAudioInputView> inputs(bus_count);
        for (std::size_t i = 0; i < bus_count; ++i)
            inputs[i] = {left[i].data(), right[i].data()};

        track_bus_indices_.swap(track_bus_indices);
        gains_.swap(gains);
        pans_.swap(pans);
        flags_.swap(flags);
        bus_left_.swap(left);
        bus_right_.swap(right);
        bus_inputs_.swap(inputs);
        max_frames_ = max_frames;
        configured_ = true;
        return true;
    } catch (...) {
        return false;
    }
}

bool RealtimeTrackBusRouter::enqueue_track_control(std::size_t track_index, float gain,
                                                   float pan, bool mute, bool solo) noexcept {
    if (!configured_ || track_index >= gains_.size() ||
        track_index > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) ||
        !std::isfinite(gain) || gain < 0.0F || gain > 2.0F ||
        !std::isfinite(pan) || pan < -1.0F || pan > 1.0F)
        return false;
    const auto flags = static_cast<std::uint8_t>((mute ? 1U : 0U) | (solo ? 2U : 0U));
    return commands_.try_push({static_cast<std::uint32_t>(track_index), gain, pan, flags});
}

bool RealtimeTrackBusRouter::process(const float* const* mono_tracks,
                                     std::size_t track_count,
                                     std::size_t frames) noexcept {
    if (!configured_ || track_count != track_bus_indices_.size() || mono_tracks == nullptr ||
        frames > max_frames_) return false;
    for (std::size_t i = 0; i < track_count; ++i) {
        if (mono_tracks[i] == nullptr) return false;
    }

    TrackCommand command{};
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
    for (std::size_t bus = 0; bus < bus_left_.size(); ++bus) {
        std::fill_n(bus_left_[bus].data(), frames, 0.0F);
        std::fill_n(bus_right_[bus].data(), frames, 0.0F);
    }

    constexpr float kQuarterPi = 0.78539816339744830962F;
    for (std::size_t track = 0; track < track_count; ++track) {
        const auto flags = flags_[track];
        // Mute always wins. If any track is soloed, non-solo tracks are excluded.
        if ((flags & 1U) != 0U || (any_solo && (flags & 2U) == 0U)) continue;
        const float angle = (pans_[track] + 1.0F) * kQuarterPi;
        const float left_gain = gains_[track] * std::cos(angle);
        const float right_gain = gains_[track] * std::sin(angle);
        auto& left = bus_left_[track_bus_indices_[track]];
        auto& right = bus_right_[track_bus_indices_[track]];
        for (std::size_t frame = 0; frame < frames; ++frame) {
            const float sample = mono_tracks[track][frame];
            left[frame] += sample * left_gain;
            right[frame] += sample * right_gain;
        }
    }
    return true;
}

} // namespace hush
