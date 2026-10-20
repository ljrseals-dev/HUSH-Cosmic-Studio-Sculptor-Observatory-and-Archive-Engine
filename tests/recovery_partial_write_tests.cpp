#include "hush/recovery_journal.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace {
bool seed(const std::filesystem::path& path) {
    hush::RecoveryJournal journal(path);
    hush::Project project;
    project.id = "last-complete-record";
    project.tracks.push_back({"t1", "safe", 0.8F, 0.1F, false, false});
    hush::Error error;
    return journal.append_snapshot(project, error);
}
bool restores_previous(const std::filesystem::path& path) {
    hush::RecoveryJournal journal(path);
    hush::Project recovered;
    hush::Error error;
    return journal.recover_latest(recovered, error) && recovered.id == "last-complete-record";
}
}

int main() {
    const auto base = std::filesystem::temp_directory_path();
    const auto header_path = base / "hush-partial-header-test.journal";
    const auto payload_path = base / "hush-partial-payload-test.journal";
    const auto malformed_path = base / "hush-malformed-complete-record-test.journal";
    std::error_code ec;
    std::filesystem::remove(header_path, ec);
    std::filesystem::remove(payload_path, ec);
    std::filesystem::remove(malformed_path, ec);

    if (!seed(header_path)) return 1;
    {
        std::ofstream file(header_path, std::ios::binary | std::ios::app);
        file.write("HUSHJ1 123", 10); // no newline: interrupted header
        file.flush();
        if (!file) return 2;
    }
    if (!restores_previous(header_path)) return 3;

    if (!seed(payload_path)) return 4;
    {
        std::ofstream file(payload_path, std::ios::binary | std::ios::app);
        file << "HUSHJ1 50\n";
        file.write("{\"schema_version\":", 18); // shorter than declared payload
        file.flush();
        if (!file) return 5;
    }
    if (!restores_previous(payload_path)) return 6;

    // A complete but invalid snapshot has trustworthy framing. Recovery should
    // skip it and still discover a later valid snapshot, rather than stopping at
    // the first parse failure and returning stale project state.
    if (!seed(malformed_path)) return 7;
    {
        const std::string malformed = R"({"schema_version":2})";
        std::ofstream file(malformed_path, std::ios::binary | std::ios::app);
        file << "HUSHJ1 " << malformed.size() << '\\n';
        file.write(malformed.data(), static_cast<std::streamsize>(malformed.size()));
        file.put('\\n');
        file.flush();
        if (!file) return 8;
    }
    {
        hush::RecoveryJournal journal(malformed_path);
        hush::Project newer;
        newer.id = "valid-record-after-malformed";
        newer.tracks.push_back({"t2", "newer", 0.6F, -0.2F, false, false});
        hush::Error error;
        if (!journal.append_snapshot(newer, error)) return 9;
    }
    {
        hush::RecoveryJournal journal(malformed_path);
        hush::Project recovered;
        hush::Error error;
        if (!journal.recover_latest(recovered, error)) return 10;
        if (recovered.id != "valid-record-after-malformed") return 11;
    }

    std::filesystem::remove(header_path, ec);
    std::filesystem::remove(payload_path, ec);
    std::filesystem::remove(malformed_path, ec);
    return 0;
}
