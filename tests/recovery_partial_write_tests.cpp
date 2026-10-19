#include "hush/recovery_journal.hpp"

#include <filesystem>
#include <fstream>
#include <string>

int main() {
    using namespace hush;
    const auto path = std::filesystem::temp_directory_path() / "hush-partial-write-fault-test.journal";
    std::error_code ec;
    std::filesystem::remove(path, ec);
    RecoveryJournal journal(path);
    Project original;
    original.id = "last-complete-record";
    original.tracks.push_back({"t1", "safe", 0.8F, 0.1F, false, false});
    Error error;
    if (!journal.append_snapshot(original, error)) return 1;

    // Fault injection: append an incomplete header as if the process stopped mid-write.
    {
        std::ofstream file(path, std::ios::binary | std::ios::app);
        file.write("HUSHJ1 123", 10);
        file.flush();
        if (!file) return 2;
    }
    Project recovered;
    if (!journal.recover_latest(recovered, error)) return 3;
    if (recovered.id != original.id) return 4;

    // Fault injection: a syntactically complete length header with a truncated payload.
    {
        std::ofstream file(path, std::ios::binary | std::ios::app);
        file << "HUSHJ1 50\n";
        file.write("{\"schema_version\":", 18);
        file.flush();
        if (!file) return 5;
    }
    Project recovered_again;
    if (!journal.recover_latest(recovered_again, error)) return 6;
    if (recovered_again.id != original.id) return 7;
    std::filesystem::remove(path, ec);
    return 0;
}
