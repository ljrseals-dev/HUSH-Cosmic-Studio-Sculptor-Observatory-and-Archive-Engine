#pragma once

#include "hush/project.hpp"
#include "hush/realtime_bus_engine_host.hpp"
#include "hush/realtime_track_bus_router.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace hush {

// Binds persisted project bus topology and stable track IDs to the realtime path.
// configure() and enqueue_track_control() belong to one control/UI thread;
// process() is the single audio-reader entry point. All routing scratch is prepared
// before audio starts. The callback path performs no allocation, locks, logging, or I/O.
class RealtimeProjectMixer {
public:
    RealtimeProjectMixer() = default;
    RealtimeProjectMixer(const RealtimeProjectMixer&) = delete;
    RealtimeProjectMixer& operator=(const RealtimeProjectMixer&) = delete;

    // track_bus_ids must contain one existing project bus ID per project track.
    // master_bus_id must identify the output/master bus in project.buses.
    // This API intentionally requires explicit routing instead of silently guessing
    // how tracks should connect to the persisted graph.
    bool configure(const Project& project,
                   const std::vector<std::string>& track_bus_ids,
                   const std::string& master_bus_id,
                   std::size_t max_frames,
                   Error& error);

    [[nodiscard]] bool enqueue_track_control(const std::string& stable_track_id,
                                             float gain, float pan,
                                             bool mute, bool solo) noexcept;

    [[nodiscard]] RealtimeProcessStatus process(const float* const* mono_tracks,
                                                std::size_t track_count,
                                                std::size_t frames,
                                                float* output_left,
                                                float* output_right) noexcept;

    // Control thread only. Call periodically and never from the audio callback.
    void reclaim_retired() noexcept { engine_.reclaim_retired(); }

    [[nodiscard]] bool configured() const noexcept { return configured_; }
    [[nodiscard]] std::size_t track_count() const noexcept { return track_ids_.size(); }
    [[nodiscard]] std::size_t bus_count() const noexcept { return bus_count_; }
    [[nodiscard]] std::size_t max_frames() const noexcept { return max_frames_; }

private:
    RealtimeTrackBusRouter router_;
    RealtimeBusEngineHost engine_;
    std::vector<std::string> track_ids_;
    std::size_t bus_count_{0};
    std::size_t max_frames_{0};
    bool configured_{false};
};

} // namespace hush
