#include "hush/project.hpp"

#include <cassert>
#include <string>

int main() {
    using namespace hush;
    Project project;
    project.id = "timeline-contract";
    project.tracks.push_back({"track-vox", "Vocals", 0.8F, 0.0F, false, false});
    project.media_sources.push_back({"source-take-1", "Media/take-1.wav", 48000U, 2U, 96000U});
    project.clips.push_back({"clip-vox-1", "track-vox", "source-take-1",
                             24000, 1200U, 48000U, 0.75F, 120U, 240U});

    const auto json = to_json(project);
    Project restored;
    Error error;
    assert(from_json(json, restored, error));
    assert(restored == project);
    assert(restored.clips.size() == 1U);
    assert(restored.clips[0].timeline_start_frame == 24000U);
    assert(restored.clips[0].source_start_frame == 1200U);
    assert(restored.clips[0].frame_count == 48000U);
    assert(restored.clips[0].fade_in_frames == 120U);
    assert(restored.clips[0].fade_out_frames == 240U);

    // Signed pre-roll positions and optional fades round-trip without schema bump.
    Project preroll = project;
    preroll.clips[0].timeline_start_frame = -1200;
    assert(from_json(to_json(preroll), restored, error));
    assert(restored.clips[0].timeline_start_frame == -1200);

    // Older schema-v1 projects with no media_sources/clips arrays remain readable.
    const std::string legacy =
        R"({"schema_version":1,"id":"legacy","pan_law":"equal_power_minus_3_db","tracks":[{"id":"t","name":"T","gain":1,"pan":0,"mute":false,"solo":false}]})";
    assert(from_json(legacy, restored, error));
    assert(restored.media_sources.empty() && restored.clips.empty());

    // Clip references and source bounds are checked when loading project state.
    Project invalid = project;
    invalid.clips[0].source_start_frame = 95000U;
    invalid.clips[0].frame_count = 2000U;
    assert(!from_json(to_json(invalid), restored, error));
    assert(error.code == "audio_clip_invalid");

    invalid = project;
    invalid.clips[0].track_id = "missing-track";
    assert(!from_json(to_json(invalid), restored, error));
    assert(error.code == "audio_clip_invalid");

    invalid = project;
    invalid.media_sources[0].sample_rate = 0U;
    assert(!from_json(to_json(invalid), restored, error));
    assert(error.code == "media_source_invalid");

    invalid = project;
    invalid.clips[0].gain = -1.0F;
    assert(!from_json(to_json(invalid), restored, error));
    assert(error.code == "audio_clip_invalid");
    return 0;
}
