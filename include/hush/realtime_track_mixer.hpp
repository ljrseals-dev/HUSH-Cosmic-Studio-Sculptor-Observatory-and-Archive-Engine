#pragma once

#include "hush/spsc_parameter_queue.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace hush {

struct TrackControlCommand {
    std::uint32_t track_index{};
    float gain{1.0F};
    float pan{0.0F};
    std::uint8_t flags{}; // bit 0 = mute, bit 1 = solo
};

// Fixed-capacity, single-producer/single-consumer realtime track mixer.
// Configure once on the control thread before audio starts. The UI/control thread
// enqueues complete parameter snapshots; process() consumes them only at block
// boundaries. process() performs no allocation, locking, logging, or I/O.
class RealtimeTrackMixer {
public:
    static constexpr std::size_t kQueueCapacity = 2048;

    bool configure(std::size_t track_count, std::size_t max_frames) noexcept;
    [[nodiscard]] bool enqueue_control(std::size_t track_index, float gain,
                                       float pan, bool mute, bool solo) noexcept;
    [[nodiscard]] bool process(const float* const* mono_tracks,
                               std::size_t track_count,
                               float* output_left, float* output_right,
                               std::size_t frames) noexcept;
    [[nodiscard]] std::size_t track_count() const noexcept { return gains_.size(); }
    [[nodiscard]] std::size_t max_frames() const noexcept { return max_frames_; }
    // Block peak and RMS telemetry, published as lock-free float bit patterns.
    // Values are linear amplitudes and are read from the non-audio thread.
    [[nodiscard]] float peak_left() const noexcept;
    [[nodiscard]] float peak_right() const noexcept;
    [[nodiscard]] float rms_left() const noexcept;
    [[nodiscard]] float rms_right() const noexcept;

private:
    std::vector<float> gains_;
    std::vector<float> pans_;
    std::vector<std::uint8_t> flags_;
    SpscParameterQueue<TrackControlCommand, kQueueCapacity> commands_;
    std::size_t max_frames_{0};
    bool configured_{false};
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
                  "RealtimeTrackMixer meters require lock-free 32-bit atomics");
    std::atomic<std::uint32_t> peak_left_bits_{0U};
    std::atomic<std::uint32_t> peak_right_bits_{0U};
    std::atomic<std::uint32_t> rms_left_bits_{0U};
    std::atomic<std::uint32_t> rms_right_bits_{0U};
};

} // namespace hush
