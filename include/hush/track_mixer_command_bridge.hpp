#pragma once

#include "hush/project.hpp"
#include "hush/realtime_track_mixer.hpp"

#include <cstddef>
#include <string>

namespace hush {

// Shared control-thread bridge used by the GUI and headless command-path tests.
// The Project remains owned by the UI/control thread; the audio thread receives
// only complete parameter snapshots through RealtimeTrackMixer's bounded SPSC queue.
class TrackMixerCommandBridge {
public:
    [[nodiscard]] static bool enqueue_project(const Project& project,
                                              RealtimeTrackMixer& engine) noexcept {
        for (std::size_t i = 0; i < project.tracks.size(); ++i) {
            const auto& track = project.tracks[i];
            if (!engine.enqueue_control(i, track.gain, track.pan, track.mute, track.solo))
                return false;
        }
        return true;
    }

    [[nodiscard]] static bool enqueue_track(const Project& project,
                                            RealtimeTrackMixer& engine,
                                            const std::string& track_id) noexcept {
        for (std::size_t i = 0; i < project.tracks.size(); ++i) {
            const auto& track = project.tracks[i];
            if (track.id == track_id)
                return engine.enqueue_control(i, track.gain, track.pan, track.mute, track.solo);
        }
        return false;
    }

    [[nodiscard]] static bool enqueue_preview(const Project& project,
                                              RealtimeTrackMixer& engine,
                                              const std::string& track_id,
                                              float gain, float pan) noexcept {
        for (std::size_t i = 0; i < project.tracks.size(); ++i) {
            const auto& track = project.tracks[i];
            if (track.id == track_id)
                return engine.enqueue_control(i, gain, pan, track.mute, track.solo);
        }
        return false;
    }
};

} // namespace hush
