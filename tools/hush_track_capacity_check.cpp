#include "hush/mixer.hpp"
#include "hush/realtime_track_mixer.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
    using namespace hush;
    constexpr std::size_t render_frames = 128U;
    constexpr std::size_t live_capacity_frames = 4096U;

    Project project;
    project.id = "track-capacity-render-check";
    project.tracks.reserve(kMaxTracks);
    for (std::size_t i = 0; i < kMaxTracks; ++i) {
        const auto id = "capacity-track-" + std::to_string(i);
        project.tracks.push_back({id, id, 1.0F, 0.0F, false, false});
    }

    std::vector<std::vector<float>> render_inputs(
        kMaxTracks, std::vector<float>(render_frames, 0.001F));
    const auto render_start = std::chrono::steady_clock::now();
    const auto rendered = mix_tracks(project, render_inputs);
    const auto render_end = std::chrono::steady_clock::now();
    if (rendered.left.size() != render_frames || rendered.right.size() != render_frames ||
        !std::isfinite(rendered.left.front()) || !std::isfinite(rendered.right.front())) {
        std::cerr << "track_capacity_offline_render_failed\n";
        return 1;
    }

    RealtimeTrackMixer live_mixer;
    if (!live_mixer.configure(kMaxTracks, live_capacity_frames)) {
        std::cerr << "track_capacity_live_mixer_setup_failed\n";
        return 1;
    }
    // Mirror the Qt live monitor's preallocated per-track buffers and pointer array.
    std::vector<std::vector<float>> live_buffers(
        kMaxTracks, std::vector<float>(live_capacity_frames, 0.0F));
    std::vector<const float*> live_inputs(kMaxTracks, nullptr);
    for (std::size_t track = 0; track < kMaxTracks; ++track) {
        live_inputs[track] = live_buffers[track].data();
        std::fill_n(live_buffers[track].data(), render_frames, 0.001F);
    }
    std::vector<float> left(live_capacity_frames, 0.0F);
    std::vector<float> right(live_capacity_frames, 0.0F);
    if (!live_mixer.process(live_inputs.data(), live_inputs.size(),
                            left.data(), right.data(), render_frames) ||
        !std::isfinite(left.front()) || !std::isfinite(right.front())) {
        std::cerr << "track_capacity_realtime_process_failed\n";
        return 1;
    }

    const double render_ms =
        std::chrono::duration<double, std::milli>(render_end - render_start).count();
    const auto live_buffer_bytes =
        static_cast<std::uint64_t>(kMaxTracks) * live_capacity_frames * sizeof(float);
    std::cout << std::fixed << std::setprecision(3)
              << "{\n"
              << "  \"status\": \"pass\",\n"
              << "  \"tracks\": " << kMaxTracks << ",\n"
              << "  \"offline_render_frames\": " << render_frames << ",\n"
              << "  \"offline_render_wall_ms\": " << render_ms << ",\n"
              << "  \"live_buffer_frames_per_track\": " << live_capacity_frames << ",\n"
              << "  \"live_track_buffer_bytes_estimate\": " << live_buffer_bytes << ",\n"
              << "  \"realtime_process_frames\": " << render_frames << ",\n"
              << "  \"hardware_audio_test\": false\n"
              << "}\n";
    return 0;
}
