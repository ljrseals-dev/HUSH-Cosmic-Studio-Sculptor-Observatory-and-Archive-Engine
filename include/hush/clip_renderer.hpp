#pragma once

#include "hush/project.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace hush {

// Decoded source prepared on a control/offline thread. Samples are interleaved
// float PCM; channel count must match the project's MediaSource metadata.
struct DecodedMediaBuffer {
    std::string source_id;
    std::uint16_t channels{0};
    std::vector<float> interleaved_samples;
};

struct ClipRenderError {
    std::string code;
    std::string entity_id;
    std::string message;
};

// Renders clip placements into one mono buffer per project track, in project
// track order. Multi-channel sources are downmixed by arithmetic channel mean.
// Negative timeline starts are pre-roll: source frames before timeline frame 0
// are skipped. Clips extending past output_frames are truncated. Fades are
// linear and clip-relative. This offline function allocates; never call it from
// the realtime audio callback.
[[nodiscard]] bool render_project_clip_tracks(
    const Project& project,
    const std::vector<DecodedMediaBuffer>& decoded_sources,
    std::size_t output_frames,
    std::vector<std::vector<float>>& track_audio,
    ClipRenderError& error);

} // namespace hush
