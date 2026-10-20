#include "hush/clip_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace hush {
namespace {
bool fail(ClipRenderError& error, const char* code,
          const std::string& entity, const char* message) {
    error = {code, entity, message};
    return false;
}

float clip_gain_at(const AudioClip& clip, std::uint64_t frame) noexcept {
    float gain = clip.gain;
    if (clip.fade_in_frames != 0U && frame < clip.fade_in_frames) {
        const float ramp = clip.fade_in_frames <= 1U ? 1.0F :
            static_cast<float>(frame) / static_cast<float>(clip.fade_in_frames - 1U);
        gain *= ramp;
    }
    if (clip.fade_out_frames != 0U && frame >= clip.frame_count - clip.fade_out_frames) {
        const auto remaining = clip.frame_count - frame;
        const float ramp = clip.fade_out_frames <= 1U ? 0.0F :
            static_cast<float>(remaining - 1U) / static_cast<float>(clip.fade_out_frames - 1U);
        gain *= ramp;
    }
    return gain;
}
} // namespace

bool render_project_clip_tracks(
    const Project& project, const std::vector<DecodedMediaBuffer>& decoded_sources,
    std::size_t output_frames, std::vector<std::vector<float>>& track_audio,
    ClipRenderError& error) {
    error = {};
    track_audio.clear();
    if (project.schema_version != 1U)
        return fail(error, "project_schema_unsupported", project.id,
                    "Only schema-v1 projects are supported.");

    std::unordered_map<std::string, std::size_t> track_indices;
    for (std::size_t i = 0; i < project.tracks.size(); ++i) {
        if (project.tracks[i].id.empty() ||
            !track_indices.emplace(project.tracks[i].id, i).second)
            return fail(error, "track_id_invalid", project.tracks[i].id,
                        "Track IDs must be non-empty and unique.");
    }
    std::unordered_map<std::string, const MediaSource*> metadata;
    for (const auto& source : project.media_sources) {
        if (source.id.empty() || source.channels == 0U ||
            !metadata.emplace(source.id, &source).second)
            return fail(error, "media_source_invalid", source.id,
                        "Media source IDs and channel metadata must be valid.");
    }
    std::unordered_map<std::string, const DecodedMediaBuffer*> decoded;
    for (const auto& buffer : decoded_sources) {
        const auto meta = metadata.find(buffer.source_id);
        if (meta == metadata.end() || buffer.channels == 0U || buffer.channels > 8U ||
            meta->second->channels != buffer.channels ||
            buffer.interleaved_samples.size() % buffer.channels != 0U ||
            !decoded.emplace(buffer.source_id, &buffer).second)
            return fail(error, "decoded_source_invalid", buffer.source_id,
                        "Decoded source metadata, channels, or ID is invalid.");
        const auto frames = buffer.interleaved_samples.size() / buffer.channels;
        if (frames < meta->second->frame_count)
            return fail(error, "decoded_source_short", buffer.source_id,
                        "Decoded data is shorter than declared source metadata.");
    }

    track_audio.resize(project.tracks.size());
    for (auto& track : track_audio) track.assign(output_frames, 0.0F);

    for (const auto& clip : project.clips) {
        const auto track = track_indices.find(clip.track_id);
        const auto source = decoded.find(clip.source_id);
        if (track == track_indices.end())
            return fail(error, "audio_clip_track_missing", clip.id, "Clip track is missing.");
        if (source == decoded.end())
            return fail(error, "audio_clip_source_missing", clip.id,
                        "A decoded buffer is required for each rendered clip.");
        const auto* meta = metadata.at(clip.source_id);
        if (clip.frame_count == 0U || !std::isfinite(clip.gain) || clip.gain < 0.0F ||
            clip.fade_in_frames > clip.frame_count || clip.fade_out_frames > clip.frame_count ||
            clip.source_start_frame > meta->frame_count ||
            clip.frame_count > meta->frame_count - clip.source_start_frame)
            return fail(error, "audio_clip_invalid", clip.id,
                        "Clip source range, gain, or fade lengths are invalid.");

        std::uint64_t first_clip_frame = 0U;
        std::size_t first_output_frame = 0U;
        if (clip.timeline_start_frame < 0) {
            const auto preroll = static_cast<std::uint64_t>(-(clip.timeline_start_frame + 1)) + 1U;
            first_clip_frame = std::min(preroll, clip.frame_count);
        } else {
            const auto start = static_cast<std::uint64_t>(clip.timeline_start_frame);
            if (start >= output_frames) continue;
            first_output_frame = static_cast<std::size_t>(start);
        }
        if (first_clip_frame >= clip.frame_count || first_output_frame >= output_frames) continue;
        const auto visible = std::min<std::uint64_t>(
            clip.frame_count - first_clip_frame,
            static_cast<std::uint64_t>(output_frames - first_output_frame));
        const auto& buffer = *source->second;
        auto& destination = track_audio[track->second];
        for (std::uint64_t n = 0; n < visible; ++n) {
            const auto clip_frame = first_clip_frame + n;
            const auto source_frame = clip.source_start_frame + clip_frame;
            const auto offset = static_cast<std::size_t>(source_frame) * buffer.channels;
            float sample = 0.0F;
            for (std::uint16_t channel = 0; channel < buffer.channels; ++channel)
                sample += buffer.interleaved_samples[offset + channel];
            sample /= static_cast<float>(buffer.channels);
            destination[first_output_frame + static_cast<std::size_t>(n)] +=
                sample * clip_gain_at(clip, clip_frame);
        }
    }
    error = {};
    return true;
}
} // namespace hush
