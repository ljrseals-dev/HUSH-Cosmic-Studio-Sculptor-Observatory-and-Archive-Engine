#include "hush/clip_bus_renderer.hpp"

#include "hush/mixer.hpp"

#include <algorithm>
#include <unordered_map>
#include <utility>

namespace hush {
namespace {
bool fail(ClipBusRenderError& error, const char* code,
          const std::string& id, const char* message) {
    error = {code, id, message};
    return false;
}
} // namespace

bool render_project_clips_to_bus_inputs(
    const Project& project,
    const std::vector<DecodedMediaBuffer>& decoded_sources,
    std::size_t output_frames,
    std::vector<BusAudioInput>& bus_inputs,
    ClipBusRenderError& error) {
    error = {};
    bus_inputs.clear();
    if (project.schema_version != 1U)
        return fail(error, "project_schema_unsupported", project.id,
                    "Only schema-v1 projects are supported.");
    if (project.buses.empty())
        return fail(error, "project_buses_missing", project.id,
                    "At least one bus is required to route rendered clips.");

    std::unordered_map<std::string, std::size_t> bus_index;
    bus_index.reserve(project.buses.size());
    for (std::size_t i = 0; i < project.buses.size(); ++i) {
        if (project.buses[i].id.empty() ||
            !bus_index.emplace(project.buses[i].id, i).second)
            return fail(error, "bus_id_invalid", project.buses[i].id,
                        "Bus IDs must be non-empty and unique.");
    }

    std::vector<std::vector<float>> mono_tracks;
    ClipRenderError clip_error;
    if (!render_project_clip_tracks(project, decoded_sources, output_frames,
                                    mono_tracks, clip_error)) {
        error = {clip_error.code, clip_error.entity_id, clip_error.message};
        return false;
    }

    bus_inputs.reserve(project.buses.size());
    for (const auto& bus : project.buses) {
        BusAudioInput input;
        input.bus_id = bus.id;
        input.audio.left.assign(output_frames, 0.0F);
        input.audio.right.assign(output_frames, 0.0F);
        bus_inputs.push_back(std::move(input));
    }

    const bool any_solo = std::any_of(project.tracks.begin(), project.tracks.end(),
        [](const Track& track) { return track.solo; });

    for (std::size_t i = 0; i < project.tracks.size(); ++i) {
        const auto& track = project.tracks[i];
        if (track.mute || (any_solo && !track.solo)) continue;

        const std::string& destination_id =
            track.bus_id.empty() ? project.buses.front().id : track.bus_id;
        const auto destination = bus_index.find(destination_id);
        if (destination == bus_index.end())
            return fail(error, "track_bus_missing", track.id,
                        "Track references a bus that is not in the project.");

        // Reuse the canonical track mixer for gain, pan law, and channel law.
        // Solo is evaluated globally above before the one-track mix is formed.
        Project single_track = project;
        single_track.tracks.assign(1U, track);
        single_track.tracks.front().mute = false;
        single_track.tracks.front().solo = false;
        single_track.buses.clear();
        single_track.routes.clear();
        const std::vector<std::vector<float>> one_track{mono_tracks[i]};
        const auto stereo = mix_tracks(single_track, one_track);
        auto& target = bus_inputs[destination->second].audio;
        for (std::size_t frame = 0; frame < output_frames; ++frame) {
            target.left[frame] += stereo.left[frame];
            target.right[frame] += stereo.right[frame];
        }
    }

    error = {};
    return true;
}

} // namespace hush
