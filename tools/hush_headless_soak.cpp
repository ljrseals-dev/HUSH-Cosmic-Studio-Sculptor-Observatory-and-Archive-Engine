#include "hush/realtime_track_mixer.hpp"
#include "hush/recovery_journal.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#ifdef __linux__
#include <fstream>
#endif

namespace {
std::uint64_t resident_bytes() {
#ifdef __linux__
    std::ifstream statm("/proc/self/statm");
    std::uint64_t pages_total = 0, pages_resident = 0;
    if (statm >> pages_total >> pages_resident) {
        (void)pages_total;
        return pages_resident * 4096ULL;
    }
#endif
    return 0; // unavailable on this platform; reported explicitly as zero/unavailable.
}
}
int main(int argc, char** argv) {
    std::uint64_t seconds = 8ULL * 60ULL * 60ULL;
    std::filesystem::path journal = "hush-soak.journal";
    if (argc > 1) seconds = std::strtoull(argv[1], nullptr, 10);
    if (argc > 2) journal = argv[2];
    if (seconds == 0 || seconds > 7ULL * 24ULL * 60ULL * 60ULL) {
        std::cerr << "usage: hush_headless_soak [seconds:1..604800] [journal-path]\n";
        return 2;
    }

    constexpr std::size_t tracks = 500, frames = 128;
    hush::RealtimeTrackMixer mixer;
    if (!mixer.configure(tracks, frames)) return 1;
    std::vector<std::vector<float>> owned(tracks, std::vector<float>(frames, 0.0F));
    std::vector<const float*> inputs(tracks);
    for (std::size_t i = 0; i < tracks; ++i) inputs[i] = owned[i].data();
    std::vector<float> left(frames), right(frames);
    hush::Project project;
    project.id = "headless-soak";
    project.tracks.reserve(tracks);
    for (std::size_t i = 0; i < tracks; ++i)
        project.tracks.push_back({"track-" + std::to_string(i), "Synthetic " + std::to_string(i), 1.0F, 0.0F, false, false});
    const hush::RecoveryJournal recovery(journal);
    hush::Error error;
    const auto start = std::chrono::steady_clock::now();
    const auto deadline = start + std::chrono::seconds(seconds);
    const auto rss_start = resident_bytes();
    std::uint64_t blocks = 0, journal_writes = 0, rejected = 0;
    auto next_snapshot = start;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto track = static_cast<std::size_t>(blocks % tracks);
        if (!mixer.enqueue_control(track, 0.75F, 0.0F, (blocks % 17U) == 0U, (blocks % 31U) == 0U)) ++rejected;
        if (!mixer.process(inputs.data(), inputs.size(), left.data(), right.data(), frames)) {
            std::cerr << "mixer_process_failed\n";
            return 1;
        }
        ++blocks;
        if (std::chrono::steady_clock::now() >= next_snapshot) {
            if (!recovery.append_snapshot(project, error)) {
                std::cerr << "journal_write_failed: " << error.code << ": " << error.message << '\n';
                return 1;
            }
            ++journal_writes;
            next_snapshot = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        }
    }
    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    const auto rss_end = resident_bytes();
    const auto drift = rss_end >= rss_start ? rss_end - rss_start : 0;
    std::cout << "{\n"
              << "  \"status\": \"completed\",\n"
              << "  \"requested_seconds\": " << seconds << ",\n"
              << "  \"elapsed_seconds\": " << elapsed << ",\n"
              << "  \"track_count\": " << tracks << ",\n"
              << "  \"blocks_processed\": " << blocks << ",\n"
              << "  \"journal_snapshots\": " << journal_writes << ",\n"
              << "  \"rejected_parameter_updates\": " << rejected << ",\n"
              << "  \"rss_start_bytes_linux_or_zero_if_unavailable\": " << rss_start << ",\n"
              << "  \"rss_end_bytes_linux_or_zero_if_unavailable\": " << rss_end << ",\n"
              << "  \"rss_positive_drift_bytes\": " << drift << ",\n"
              << "  \"journal_path\": \"" << journal.string() << "\",\n"
              << "  \"hardware_audio_test\": false,\n"
              << "  \"pass_threshold_configured\": false\n"
              << "}\n";
    return 0;
}
