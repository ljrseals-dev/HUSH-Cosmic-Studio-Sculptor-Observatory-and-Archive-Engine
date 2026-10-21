#include "hush/realtime_project_audio_pipeline.hpp"

#include <cassert>
#include <cmath>
#include <string>

namespace {
bool near(float a, float b) { return std::abs(a - b) < 1.0e-4F; }
}

int main() {
    using namespace hush;
    Project project;
    project.id = "pipeline-test";
    project.tracks = {
        {"kick", "Kick", 1.0F, -1.0F, false, false, "music"},
        {"vocal", "Vocal", 1.0F, 1.0F, false, false, "music"}
    };
    project.buses = {{"capture", "Capture"}, {"music", "Music"}, {"master", "Master"}};
    project.routes = {{"music", "master", SendTap::PostFader, 1.0F}};

    RealtimeProjectAudioPipeline pipeline;
    std::string error;
    assert(pipeline.configure(project, 8U, error));
    assert(pipeline.input_bus_id() == "capture");
    assert(pipeline.master_bus_id() == "master");

    const float input[] = {1.0F, 0.25F, 0.5F, 0.5F};
    float output[4]{};
    assert(pipeline.process(input, output, 2U));
    assert(near(output[0], 1.0F));
    assert(near(output[1], 0.25F));
    assert(near(output[2], 0.5F));
    assert(near(output[3], 0.5F));

    // A queued mute is applied at the next processing-block boundary.
    assert(pipeline.enqueue_track_control(0U, 1.0F, -1.0F, true, false));
    assert(pipeline.process(input, output, 2U));
    assert(near(output[0], 0.0F));
    assert(near(output[1], 0.25F));

    // Explicit track bus IDs must resolve to a bus in the validated project graph.
    Project invalid = project;
    invalid.tracks[1].bus_id = "not-a-bus";
    RealtimeProjectAudioPipeline rejected;
    assert(!rejected.configure(invalid, 8U, error));
    assert(error.find("track_bus_unknown") != std::string::npos);

    // Old schema-v1 projects without buses or per-track assignments retain a
    // capture -> master fallback and a centered equal-power track mix.
    Project legacy;
    legacy.tracks = {{"legacy-track", "Legacy", 1.0F, 0.0F, false, false}};
    RealtimeProjectAudioPipeline fallback;
    assert(fallback.configure(legacy, 8U, error));
    const float mono_input[] = {1.0F, 0.0F};
    float fallback_output[2]{};
    assert(fallback.process(mono_input, fallback_output, 1U));
    assert(near(fallback_output[0], std::sqrt(0.5F)));
    assert(near(fallback_output[1], std::sqrt(0.5F)));
    return 0;
}
