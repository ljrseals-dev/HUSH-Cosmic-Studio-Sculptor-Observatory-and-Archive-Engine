#pragma once

#include "hush/realtime_bus_engine.hpp"
#include "hush/spsc_parameter_queue.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace hush {

// Runtime track-to-bus stage. The immutable mapping is built on the control thread;
// control values cross a bounded SPSC queue and are applied at block boundaries.
// Configure once before audio starts. process() does not allocate, lock, log, or do I/O.
class RealtimeTrackBusRouter {
public:
    static constexpr std::size_t kQueueCapacity = 2048;

    bool configure(std::vector<std::size_t> track_bus_indices,
                   std::size_t bus_count, std::size_t max_frames) noexcept;
    [[nodiscard]] bool enqueue_track_control(std::size_t track_index, float gain,
                                             float pan, bool mute, bool solo) noexcept;
    [[nodiscard]] bool process(const float* const* mono_tracks,
                               std::size_t track_count,
                               std::size_t frames) noexcept;

    [[nodiscard]] const BusAudioInputView* bus_inputs() const noexcept {
        return bus_inputs_.data();
    }
    [[nodiscard]] std::size_t bus_count() const noexcept { return bus_inputs_.size(); }
    [[nodiscard]] std::size_t track_count() const noexcept { return track_bus_indices_.size(); }
    [[nodiscard]] std::size_t max_frames() const noexcept { return max_frames_; }

private:
    struct TrackCommand {
        std::uint32_t track_index{};
        float gain{1.0F};
        float pan{0.0F};
        std::uint8_t flags{}; // bit 0 = mute, bit 1 = solo
    };

    std::vector<std::size_t> track_bus_indices_;
    std::vector<float> gains_;
    std::vector<float> pans_;
    std::vector<std::uint8_t> flags_;
    std::vector<std::vector<float>> bus_left_;
    std::vector<std::vector<float>> bus_right_;
    std::vector<BusAudioInputView> bus_inputs_;
    SpscParameterQueue<TrackCommand, kQueueCapacity> commands_;
    std::size_t max_frames_{0};
    bool configured_{false};
};

} // namespace hush
