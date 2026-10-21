#pragma once

#include "hush/project.hpp"

#include <string>

namespace hush {

// Control-thread seam between Qt mixer edits and the live engine. Implementations
// enqueue bounded commands only; they never process audio on the GUI thread.
// A deterministic fake sink can exercise the same widget-to-command path without
// opening an ALSA device.
class LiveTrackControlSink {
public:
    virtual ~LiveTrackControlSink() = default;

    [[nodiscard]] virtual bool running() const noexcept = 0;
    [[nodiscard]] virtual bool enqueue_track_control_values(
        const Project& project, const std::string& track_id,
        float gain, float pan, bool mute, bool solo) noexcept = 0;
};

} // namespace hush
