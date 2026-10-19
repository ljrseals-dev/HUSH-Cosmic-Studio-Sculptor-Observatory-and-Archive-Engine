#include "hush/realtime_track_mixer.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    constexpr std::size_t kDefaultSamples = 10'000'000;
    constexpr std::size_t kBlockFrames = 512;
    constexpr std::size_t kSampleRate = 48'000;
    const std::size_t total_samples = argc > 1
        ? static_cast<std::size_t>(std::strtoull(argv[1], nullptr, 10))
        : kDefaultSamples;
    if (total_samples == 0U) {
        std::cerr << "sample count must be greater than zero\n";
        return 2;
    }

    // All allocation and configuration happens before the timed processing region.
    hush::RealtimeTrackMixer mixer;
    if (!mixer.configure(1U, kBlockFrames)) {
        std::cerr << "could not configure one-track benchmark mixer\n";
        return 2;
    }
    std::vector<float> input(kBlockFrames);
    std::vector<float> left(kBlockFrames);
    std::vector<float> right(kBlockFrames);
    const float* tracks[] = {input.data()};
    for (std::size_t i = 0; i < kBlockFrames; ++i) {
        // Deterministic bounded synthetic voice-like test signal.
        input[i] = 0.20F * static_cast<float>((static_cast<int>(i % 97U) - 48)) / 48.0F;
    }

    const auto start = std::chrono::steady_clock::now();
    std::size_t processed = 0;
    while (processed < total_samples) {
        const std::size_t frames = std::min(kBlockFrames, total_samples - processed);
        if (!mixer.process(tracks, 1U, left.data(), right.data(), frames)) {
            std::cerr << "mixer process rejected a benchmark block\n";
            return 3;
        }
        // Keep the output observable to prevent the benchmark from being optimized away.
        volatile float sink = left[frames - 1U] + right[frames - 1U];
        (void)sink;
        processed += frames;
    }
    const auto stop = std::chrono::steady_clock::now();
    const double seconds = std::chrono::duration<double>(stop - start).count();
    const double audio_seconds = static_cast<double>(processed) / kSampleRate;
    const double realtime_factor = seconds > 0.0 ? audio_seconds / seconds : 0.0;

    std::cout << std::fixed << std::setprecision(6)
              << "{\"path\":\"RealtimeTrackMixer one-track baseline\","
              << "\"sample_rate_hz\":" << kSampleRate << ','
              << "\"block_frames\":" << kBlockFrames << ','
              << "\"samples_processed\":" << processed << ','
              << "\"wall_seconds\":" << seconds << ','
              << "\"audio_seconds\":" << audio_seconds << ','
              << "\"realtime_factor\":" << realtime_factor << "}\n";
    return 0;
}
