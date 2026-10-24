#pragma once

#include "hush/realtime_bus_engine.hpp"
#include "hush/realtime_track_mixer.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace hush {

// Control-thread-configured first stage of the track -> bus -> master pipeline.
// Track-to-bus assignments are explicit runtime topology until the project schema
// defines a persisted track output-bus field. Configure before audio starts.
// process() is single-consumer, allocation-free, and applies controls at block
// boundaries. The caller passes bus_inputs() to RealtimeBusEngine::process().
class RealtimeTrackBusRouter {
public:
    static constexpr std::size_t kQueueCapacity = 2048;

    bool configure(const std::vector<std::size_t>& track_bus_indices,
                   std::size_t bus_count, std::size_t max_frames) noexcept;

    [[nodiscard]] bool enqueue_control(std::size_t track_index, float gain,
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
    std::vector<std::size_t> track_bus_indices_;
    std::vector<float> gains_;
    std::vector<float> pans_;
    std::vector<std::uint8_t> flags_;
    std::vector<std::vector<float>> left_;
    std::vector<std::vector<float>> right_;
    std::vector<BusAudioInputView> bus_inputs_;
    SpscParameterQueue<TrackControlCommand, kQueueCapacity> commands_;
    std::size_t max_frames_{0};
    bool configured_{false};
};

} // namespace hush
