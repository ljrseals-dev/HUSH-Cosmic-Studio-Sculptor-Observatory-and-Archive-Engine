#include "hush/realtime_track_mixer.hpp"
#include "hush/recovery_journal.hpp"

#include <algorithm>
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
    return 0; // unavailable on this platform; reported with rss_available=false.
}
std::string json_escape(const std::string& value) {
    std::string out;
    for (const unsigned char c : value) {
        if (c == '\\' || c == '"') { out.push_back('\\'); out.push_back(static_cast<char>(c)); }
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c < 0x20U) out += '?';
        else out.push_back(static_cast<char>(c));
    }
    return out;
}
}
int main(int argc, char** argv) {
    std::uint64_t seconds = 8ULL * 60ULL * 60ULL;
    std::filesystem::path journal = "hush-soak.journal";
    std::uint64_t max_rss_drift_mb = 64;
    if (argc > 1) seconds = std::strtoull(argv[1], nullptr, 10);
    if (argc > 2) journal = argv[2];
    if (argc > 3) max_rss_drift_mb = std::strtoull(argv[3], nullptr, 10);
    if (seconds == 0 || seconds > 7ULL * 24ULL * 60ULL * 60ULL || max_rss_drift_mb > 1048576ULL) {
        std::cerr << "usage: hush_headless_soak [seconds:1..604800] [journal-path] [max-positive-rss-drift-mb:0..1048576]\n";
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
    const bool rss_available = rss_start != 0U && rss_end != 0U;
    const std::uint64_t max_drift_bytes = max_rss_drift_mb * 1024ULL * 1024ULL;
    const bool passed = rss_available && drift <= max_drift_bytes;
    const char* status = !rss_available ? "not_measured" : (passed ? "pass" : "fail");
    std::cout << "{\n"
              << "  \"status\": \"" << status << "\",\n"
              << "  \"requested_seconds\": " << seconds << ",\n"
              << "  \"elapsed_seconds\": " << elapsed << ",\n"
              << "  \"track_count\": " << tracks << ",\n"
              << "  \"blocks_processed\": " << blocks << ",\n"
              << "  \"journal_snapshots\": " << journal_writes << ",\n"
              << "  \"rejected_parameter_updates\": " << rejected << ",\n"
              << "  \"rss_available\": " << (rss_available ? "true" : "false") << ",\n"
              << "  \"rss_start_bytes\": " << rss_start << ",\n"
              << "  \"rss_end_bytes\": " << rss_end << ",\n"
              << "  \"rss_positive_drift_bytes\": " << drift << ",\n"
              << "  \"max_positive_rss_drift_mb\": " << max_rss_drift_mb << ",\n"
              << "  \"journal_path\": \"" << json_escape(journal.string()) << "\",\n"
              << "  \"hardware_audio_test\": false,\n"
              << "  \"pass_threshold_configured\": true\n"
              << "}\n";
    return !rss_available ? 4 : (passed ? 0 : 3);
}
