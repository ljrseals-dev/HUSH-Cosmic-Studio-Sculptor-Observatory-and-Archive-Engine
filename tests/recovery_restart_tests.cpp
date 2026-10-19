#include "hush/recovery_journal.hpp"
#include <cstdlib>
#include <filesystem>
#include <string>

int main(int argc, char** argv) {
    using namespace hush;
    if (argc == 3 && std::string(argv[1]) == "--crash-child") {
        RecoveryJournal journal(std::filesystem::path(argv[2]));
        Project snapshot;
        snapshot.id = "durable-child-snapshot";
        snapshot.tracks.push_back({"track-child", "Child", 0.75F, -0.25F, false, false});
        Error error;
        const bool saved = journal.append_snapshot(snapshot, error);
        // Exit without normal C++ teardown to model abrupt process termination after
        // append_snapshot has returned (and requested stable-storage sync).
        std::_Exit(saved ? 0 : 2);
    }

    const auto journal_path =
        std::filesystem::temp_directory_path() / "hush-recovery-process-restart.journal";
    std::error_code ec;
    std::filesystem::remove(journal_path, ec);

    const auto executable = std::filesystem::absolute(argv[0]).string();
    const std::string command = "\"" + executable + "\" --crash-child \"" +
                                journal_path.string() + "\"";
    const int child_status = std::system(command.c_str());
    if (child_status != 0) {
        std::filesystem::remove(journal_path, ec);
        return 1;
    }

    // A fresh process context opens the journal and must restore the child's last
    // completed snapshot. This verifies process exit/restart, not power-loss safety.
    RecoveryJournal journal(journal_path);
    Project recovered;
    Error error;
    const bool restored = journal.recover_latest(recovered, error);
    std::filesystem::remove(journal_path, ec);
    if (!restored) return 2;
    if (recovered.id != "durable-child-snapshot") return 3;
    if (recovered.tracks.size() != 1 || recovered.tracks.front().id != "track-child") return 4;
    return 0;
}
