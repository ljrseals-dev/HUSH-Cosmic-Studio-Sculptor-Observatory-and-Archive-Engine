#include "hush/realtime_track_mixer.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    std::size_t tracks = 500;
    std::size_t seconds = 10;
    if (argc > 1) tracks = static_cast<std::size_t>(std::strtoull(argv[1], nullptr, 10));
    if (argc > 2) seconds = static_cast<std::size_t>(std::strtoull(argv[2], nullptr, 10));
    if (tracks == 0 || tracks > 100000 || seconds == 0 || seconds > 86400) {
        std::cerr << "usage: hush_routing_benchmark [tracks:1..100000] [seconds:1..86400]\n";
        return 2;
    }

    constexpr std::size_t frames = 128;
    hush::RealtimeTrackMixer mixer;
    if (!mixer.configure(tracks, frames)) {
        std::cerr << "mixer_configuration_failed\n";
        return 1;
    }
    std::vector<std::vector<float>> owned(tracks, std::vector<float>(frames));
    std::vector<const float*> inputs(tracks);
    std::vector<float> left(frames), right(frames);
    for (std::size_t t = 0; t < tracks; ++t) {
        inputs[t] = owned[t].data();
        for (std::size_t f = 0; f < frames; ++f)
            owned[t][f] = 0.001F * std::sin(static_cast<float>((t + 1) * (f + 1)) * 0.013F);
    }

    const auto start = std::chrono::steady_clock::now();
    const auto end = start + std::chrono::seconds(seconds);
    std::uint64_t blocks = 0;
    std::size_t rejected_updates = 0;
    while (std::chrono::steady_clock::now() < end) {
        // Deliberate parameter churn; a full bounded queue is measured, not hidden.
        const std::size_t index = static_cast<std::size_t>(blocks % tracks);
        if (!mixer.enqueue_control(index, 0.5F + static_cast<float>(blocks % 100) / 100.0F,
                                   static_cast<float>(static_cast<int>(blocks % 201) - 100) / 100.0F,
                                   false, false)) ++rejected_updates;
        if (!mixer.process(inputs.data(), inputs.size(), left.data(), right.data(), frames)) {
            std::cerr << "mixer_process_failed\n";
            return 1;
        }
        ++blocks;
    }
    const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    const double blocks_per_second = elapsed > 0.0 ? static_cast<double>(blocks) / elapsed : 0.0;
    const double realtime_block_budget_us = 1.0e6 * static_cast<double>(frames) / 48000.0;
    const double mean_block_us = blocks ? elapsed * 1.0e6 / static_cast<double>(blocks) : 0.0;
    std::cout << "{\n"
              << "  \"status\": \"completed\",\n"
              << "  \"tracks\": " << tracks << ",\n"
              << "  \"frames_per_block\": " << frames << ",\n"
              << "  \"assumed_sample_rate_hz\": 48000,\n"
              << "  \"requested_seconds\": " << seconds << ",\n"
              << "  \"elapsed_seconds\": " << elapsed << ",\n"
              << "  \"blocks\": " << blocks << ",\n"
              << "  \"blocks_per_second\": " << blocks_per_second << ",\n"
              << "  \"mean_block_processing_us\": " << mean_block_us << ",\n"
              << "  \"block_realtime_budget_us\": " << realtime_block_budget_us << ",\n"
              << "  \"rejected_parameter_updates\": " << rejected_updates << ",\n"
              << "  \"hardware_audio_test\": false\n"
              << "}\n";
    return 0;
}
