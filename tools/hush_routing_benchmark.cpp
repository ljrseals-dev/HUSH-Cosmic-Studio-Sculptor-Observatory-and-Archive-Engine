#include "hush/realtime_track_mixer.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

int main(int argc, char** argv) {
    std::size_t tracks = 500;
    std::size_t seconds = 10;
    double max_block_us = 2666.667; // 128 frames at 48 kHz.
    if (argc > 1) tracks = static_cast<std::size_t>(std::strtoull(argv[1], nullptr, 10));
    if (argc > 2) seconds = static_cast<std::size_t>(std::strtoull(argv[2], nullptr, 10));
    if (argc > 3) max_block_us = std::strtod(argv[3], nullptr);
    if (tracks == 0 || tracks > 100000 || seconds == 0 || seconds > 86400 ||
        !std::isfinite(max_block_us) || max_block_us <= 0.0) {
        std::cerr << "usage: hush_routing_benchmark [tracks:1..100000] [seconds:1..86400] [max_block_us>0]\n";
        return 2;
    }

    constexpr std::size_t frames = 128;
    constexpr double sample_rate = 48000.0;
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

    // Instrumentation allocations happen before/after each process call, not inside it.
    std::vector<double> process_us;
    process_us.reserve(std::min<std::size_t>(seconds * 200U, 1000000U));
    const auto start = std::chrono::steady_clock::now();
    const auto end = start + std::chrono::seconds(seconds);
    std::uint64_t blocks = 0;
    std::size_t rejected_updates = 0;
    double max_observed_us = 0.0;
    while (std::chrono::steady_clock::now() < end) {
        const std::size_t index = static_cast<std::size_t>(blocks % tracks);
        if (!mixer.enqueue_control(index, 0.5F + static_cast<float>(blocks % 100) / 100.0F,
                                   static_cast<float>(static_cast<int>(blocks % 201) - 100) / 100.0F,
                                   false, false)) ++rejected_updates;
        const auto block_start = std::chrono::steady_clock::now();
        const bool processed = mixer.process(inputs.data(), inputs.size(), left.data(), right.data(), frames);
        const auto block_stop = std::chrono::steady_clock::now();
        if (!processed) {
            std::cerr << "mixer_process_failed\n";
            return 1;
        }
        const double block_us = std::chrono::duration<double, std::micro>(block_stop - block_start).count();
        if ((blocks % 100U) == 0U && process_us.size() < 1000000U) process_us.push_back(block_us);
        max_observed_us = std::max(max_observed_us, block_us);
        ++blocks;
    }
    const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    const double mean_process_us = process_us.empty() ? 0.0 :
        std::accumulate(process_us.begin(), process_us.end(), 0.0) / static_cast<double>(process_us.size());
    std::sort(process_us.begin(), process_us.end());
    const auto percentile = [&process_us](double p) {
        if (process_us.empty()) return 0.0;
        const auto index = static_cast<std::size_t>(p * static_cast<double>(process_us.size() - 1U));
        return process_us[index];
    };
    const double blocks_per_second = elapsed > 0.0 ? static_cast<double>(blocks) / elapsed : 0.0;
    const double realtime_block_budget_us = 1.0e6 * static_cast<double>(frames) / sample_rate;
    const bool passed = !process_us.empty() && percentile(0.99) <= max_block_us;
    std::cout << std::fixed << std::setprecision(3)
              << "{\n"
              << "  \"status\": \"" << (passed ? "pass" : "fail") << "\",\n"
              << "  \"tracks\": " << tracks << ",\n"
              << "  \"frames_per_block\": " << frames << ",\n"
              << "  \"assumed_sample_rate_hz\": 48000,\n"
              << "  \"requested_seconds\": " << seconds << ",\n"
              << "  \"elapsed_seconds\": " << elapsed << ",\n"
              << "  \"blocks\": " << blocks << ",\n"
              << "  \"blocks_per_second\": " << blocks_per_second << ",\n"
              << "  \"mean_block_processing_us\": " << (blocks ? elapsed * 1.0e6 / static_cast<double>(blocks) : 0.0) << ",\n"
              << "  \"p95_process_us\": " << percentile(0.95) << ",\n"
              << "  \"p99_process_us\": " << percentile(0.99) << ",\n"
              << "  \"max_process_us\": " << max_observed_us << ",\n"
              << "  \"configured_p99_budget_us\": " << max_block_us << ",\n"
              << "  \"nominal_realtime_block_budget_us\": " << realtime_block_budget_us << ",\n"
              << "  \"rejected_parameter_updates\": " << rejected_updates << ",\n"
              << "  \"hardware_audio_test\": false\n"
              << "}\n";
    return passed ? 0 : 3;
}
