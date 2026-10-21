#include "hush/realtime_project_audio_pipeline.hpp"
#include "hush/recording_session.hpp"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
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
    assert(pipeline.bus_input_meter_count() == 3U);
    assert(pipeline.bus_output_meter_count() == 3U);

    const float input[] = {1.0F, 0.25F, 0.5F, 0.5F};
    float output[4]{};
    assert(pipeline.process(input, output, 2U));
    assert(near(output[0], 1.0F));
    assert(near(output[1], 0.25F));
    assert(near(output[2], 0.5F));
    assert(near(output[3], 0.5F));
    const auto music_meter = pipeline.bus_input_meter(1U);
    assert(near(music_meter.peak_left, 1.0F));
    assert(near(music_meter.peak_right, 0.5F));
    assert(near(music_meter.rms_left, std::sqrt(0.625F)));
    assert(near(music_meter.rms_right, std::sqrt(0.15625F)));
    const auto music_output_meter = pipeline.bus_output_meter(1U);
    assert(near(music_output_meter.peak_left, 1.0F));
    assert(near(music_output_meter.peak_right, 0.5F));
    assert(near(music_output_meter.rms_left, std::sqrt(0.625F)));
    assert(near(music_output_meter.rms_right, std::sqrt(0.15625F)));
    assert(near(pipeline.bus_input_meter(99U).peak_left, 0.0F));
    assert(near(pipeline.bus_output_meter(99U).peak_left, 0.0F));

    // A queued mute is applied at the next processing-block boundary.
    assert(pipeline.enqueue_track_control(0U, 1.0F, -1.0F, true, false));
    assert(pipeline.process(input, output, 2U));
    assert(near(output[0], 0.0F));
    assert(near(output[1], 0.25F));
    const auto muted_music_meter = pipeline.bus_input_meter(1U);
    assert(near(muted_music_meter.peak_left, 0.0F));
    assert(near(muted_music_meter.peak_right, 0.5F));

    // Bus mute is routed through the same project-aware pipeline and applies at a block boundary.
    assert(pipeline.enqueue_bus_control("music", 1.0F, true, false));
    assert(pipeline.process(input, output, 2U));
    assert(near(output[0], 0.0F) && near(output[1], 0.0F));
    assert(near(output[2], 0.0F) && near(output[3], 0.0F));
    assert(near(pipeline.bus_output_meter(1U).peak_left, 0.0F));
    assert(near(pipeline.bus_output_meter(2U).peak_right, 0.0F));
    assert(!pipeline.enqueue_bus_control("missing-bus", 1.0F, false, false));

    // Persisted inserts are activated by the project-aware live pipeline.
    Project inserted_project = project;
    inserted_project.inserts = {{"music-gain", "music", InsertKind::Gain, true, 0.5F}};
    RealtimeProjectAudioPipeline inserted_pipeline;
    assert(inserted_pipeline.configure(inserted_project, 8U, error));
    assert(inserted_pipeline.process(input, output, 2U));
    assert(near(output[0], 0.5F) && near(output[1], 0.125F));
    assert(near(output[2], 0.25F) && near(output[3], 0.25F));
    assert(near(inserted_pipeline.bus_output_meter(1U).peak_left, 0.5F));

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

    // Explicit capture recording uses the SPSC ring and finalizes a float WAV off-thread.
    const auto wav_path = std::filesystem::temp_directory_path() / "hush-pipeline-recording-test.wav";
    std::error_code cleanup_error;
    std::filesystem::remove(wav_path, cleanup_error);
    RecordingSession recording;
    assert(recording.start(wav_path, 48000U, 2U, 32U, 4U, &error));
    fallback.set_recording_session(&recording);
    assert(fallback.process(mono_input, fallback_output, 1U));
    assert(fallback.process(mono_input, fallback_output, 1U));
    fallback.set_recording_session(nullptr);
    assert(recording.stop(&error));
    assert(recording.frames_written() == 2U);
    assert(recording.dropped_frames() == 0U);
    std::ifstream wav(wav_path, std::ios::binary);
    assert(wav.good());
    char header[4]{};
    wav.read(header, 4);
    assert(header[0] == 'R' && header[1] == 'I' && header[2] == 'F' && header[3] == 'F');
    wav.close();
    std::filesystem::remove(wav_path, cleanup_error);
    return 0;
}
