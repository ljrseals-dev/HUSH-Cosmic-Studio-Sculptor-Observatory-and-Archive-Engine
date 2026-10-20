#pragma once

#include "hush/project.hpp"
#include "hush/spsc_parameter_queue.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace hush {

enum class TrackCommandType : std::uint8_t { Controls, SetTrackCount, ControlsAndTrackCount };

struct TrackControlCommand {
    std::uint32_t track_index{};
    float gain{1.0F};
    float pan{0.0F};
    std::uint8_t flags{}; // bit 0 = mute, bit 1 = solo
    TrackCommandType type{TrackCommandType::Controls};
    std::uint32_t active_track_count{};
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
    // Structural count changes are queued and become visible only at a process-block
    // boundary. The mixer remains configured to its full capacity.
    [[nodiscard]] bool enqueue_active_track_count(std::size_t active_count) noexcept;
    [[nodiscard]] bool enqueue_track_state_and_count(std::size_t track_index, float gain,
                                                     float pan, bool mute, bool solo,
                                                     std::size_t active_count) noexcept;
    [[nodiscard]] bool process(const float* const* mono_tracks,
                               std::size_t track_count,
                               float* output_left, float* output_right,
                               std::size_t frames) noexcept;
    [[nodiscard]] std::size_t track_count() const noexcept { return gains_.size(); }
    [[nodiscard]] std::size_t active_track_count() const noexcept { return active_track_count_; }
    [[nodiscard]] std::size_t max_frames() const noexcept { return max_frames_; }

private:
    std::vector<float> gains_;
    std::vector<float> pans_;
    std::vector<std::uint8_t> flags_;
    SpscParameterQueue<TrackControlCommand, kQueueCapacity> commands_;
    std::size_t max_frames_{0};
    std::size_t active_track_count_{0};
    bool configured_{false};
};

} // namespace hush
