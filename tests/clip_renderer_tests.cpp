#include "hush/clip_renderer.hpp"
#include "hush/mixer.hpp"

#include <cassert>
#include <cmath>
#include <vector>

int main() {
    using namespace hush;
    Project project;
    project.id = "clip-render-reference";
    project.tracks = {
        {"t1", "Track 1", 0.5F, -1.0F, false, false},
        {"t2", "Track 2", 1.0F, 1.0F, false, false}
    };
    project.media_sources = {
        {"a", "a.wav", 48000U, 1U, 4U},
        {"b", "b.wav", 48000U, 1U, 4U}
    };
    project.clips = {
        {"c1", "t1", "a", 0, 0, 4, 1.0F, 2, 2},
        {"c2", "t1", "b", 2, 0, 4, 0.5F},
        {"c3", "t2", "a", -2, 0, 4, 1.0F},
        {"c4", "t2", "b", 6, 0, 4, 1.0F}
    };
    const std::vector<DecodedMediaBuffer> decoded = {
        {"a", 1U, {1.0F, 2.0F, 3.0F, 4.0F}},
        {"b", 1U, {10.0F, 20.0F, 30.0F, 40.0F}}
    };
    std::vector<std::vector<float>> tracks;
    ClipRenderError error;
    assert(render_project_clip_tracks(project, decoded, 8U, tracks, error));
    assert(tracks.size() == 2U);
    const std::vector<float> expected_t1{0.0F, 2.0F, 8.0F, 10.0F, 15.0F, 20.0F, 0.0F, 0.0F};
    const std::vector<float> expected_t2{3.0F, 4.0F, 0.0F, 0.0F, 0.0F, 0.0F, 10.0F, 20.0F};
    for (std::size_t i = 0; i < 8U; ++i) {
        assert(std::abs(tracks[0][i] - expected_t1[i]) < 1.0e-6F);
        assert(std::abs(tracks[1][i] - expected_t2[i]) < 1.0e-6F);
    }
    const auto stereo = mix_tracks(project, tracks);
    assert(std::abs(stereo.left[2] - 4.0F) < 1.0e-6F);
    assert(std::abs(stereo.right[2]) < 1.0e-6F);
    assert(std::abs(stereo.right[0] - 3.0F) < 1.0e-6F);

    auto invalid = project;
    invalid.clips[0].fade_in_frames = 5U;
    assert(!render_project_clip_tracks(invalid, decoded, 8U, tracks, error));
    assert(error.code == "audio_clip_invalid");
    assert(!render_project_clip_tracks(project, {}, 8U, tracks, error));
    assert(error.code == "audio_clip_source_missing");
    return 0;
}
