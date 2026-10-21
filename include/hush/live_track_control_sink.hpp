#pragma once

#include "hush/project.hpp"

#include <string>

namespace hush {

// Narrow control-thread seam between mixer widgets and the live processing path.
// Implementations must enqueue bounded commands only; they must not process audio
// on the GUI thread. This interface also allows widget tests to exercise the
// production command bridge without opening an ALSA device.
class LiveTrackControlSink {
public:
    virtual ~LiveTrackControlSink() = default;

    [[nodiscard]] virtual bool running() const noexcept = 0;
    [[nodiscard]] virtual bool enqueue_track_control(
        const Project& project, const std::string& track_id) noexcept = 0;
    [[nodiscard]] virtual bool enqueue_track_preview(
        const Project& project, const std::string& track_id,
        float gain, float pan) noexcept = 0;
};

} // namespace hush
