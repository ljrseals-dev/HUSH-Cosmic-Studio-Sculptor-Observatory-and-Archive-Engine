#include "hush/realtime_project_mixer.hpp"

#include <cassert>
#include <cmath>
#include <string>
#include <vector>

namespace {
bool near(float a, float b) { return std::abs(a - b) < 1.0e-5F; }

hush::Project project_fixture() {
    hush::Project project;
    project.id = "project-mixer-test";
    project.tracks = {
        {"kick", "Kick", 1.0F, 0.0F, false, false},
        {"vocal", "Vocal", 0.5F, -1.0F, false, false},
    };
    project.buses = {{"drums", "Drums"}, {"vocal-bus", "Vocal Bus"}, {"master", "Master"}};
    project.routes = {
        {"drums", "master", hush::SendTap::PostFader, 1.0F},
        {"vocal-bus", "master", hush::SendTap::PostFader, 1.0F},
    };
    return project;
}
}

int main() {
    using namespace hush;
    auto project = project_fixture();
    RealtimeProjectMixer mixer;
    Error error;
    assert(!mixer.configure(project, {"drums", "missing"}, "master", 8, error));
    assert(error.code == "project_mixer_bus_assignment_invalid");
    assert(!mixer.configured());

    assert(mixer.configure(project, {"drums", "vocal-bus"}, "master", 8, error));
    assert(error.code.empty());
    assert(mixer.configured());
    assert(mixer.track_count() == 2U);
    assert(mixer.bus_count() == 3U);
    assert(mixer.max_frames() == 8U);
    assert(!mixer.enqueue_track_control("not-a-track", 1.0F, 0.0F, false, false));

    const float kick[] = {1.0F, 0.5F, -1.0F, 0.0F};
    const float vocal[] = {1.0F, 1.0F, 1.0F, 1.0F};
    const float* tracks[] = {kick, vocal};
    float left[4]{};
    float right[4]{};
    assert(mixer.process(tracks, 2, 4, left, right) == RealtimeProcessStatus::Ok);
    // Center-panned kick plus left-panned vocal at half gain, routed through
    // separate buses and summed at master.
    assert(near(left[0], 0.70710678F + 0.5F));
    assert(near(right[0], 0.70710678F));

    // Stable-ID control updates are consumed at the next audio block.
    assert(mixer.enqueue_track_control("kick", 0.0F, 0.0F, false, false));
    assert(mixer.process(tracks, 2, 4, left, right) == RealtimeProcessStatus::Ok);
    assert(near(left[0], 0.5F));
    assert(near(right[0], 0.0F));

    // A muted solo remains silent and still excludes non-solo tracks.
    assert(mixer.enqueue_track_control("kick", 1.0F, 0.0F, false, false));
    assert(mixer.enqueue_track_control("vocal", 0.5F, -1.0F, true, true));
    assert(mixer.process(tracks, 2, 4, left, right) == RealtimeProcessStatus::Ok);
    assert(near(left[0], 0.0F));
    assert(near(right[0], 0.0F));

    // An unmuted solo routes only the selected track.
    assert(mixer.enqueue_track_control("vocal", 0.5F, -1.0F, false, true));
    assert(mixer.process(tracks, 2, 4, left, right) == RealtimeProcessStatus::Ok);
    assert(near(left[0], 0.5F));
    assert(near(right[0], 0.0F));

    // Invalid master selection is rejected before a separate mixer becomes usable.
    RealtimeProjectMixer invalid_master;
    assert(!invalid_master.configure(project, {"drums", "vocal-bus"}, "missing-master", 8, error));
    assert(!invalid_master.configured());
    assert(mixer.process(tracks, 2, 4, left, right) == RealtimeProcessStatus::Ok);

    assert(mixer.process(tracks, 1, 4, left, right) == RealtimeProcessStatus::InvalidInputCount);
    assert(mixer.process(tracks, 2, 9, left, right) == RealtimeProcessStatus::FrameCountExceedsCapacity);
    return 0;
}
