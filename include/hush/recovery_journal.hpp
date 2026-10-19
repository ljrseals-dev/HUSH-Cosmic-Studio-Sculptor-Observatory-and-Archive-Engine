#pragma once
#include "hush/project.hpp"
#include <filesystem>
#include <string>
#include <utility>

namespace hush {
// Append-only, local recovery journal. Records are length-framed so a torn final write
// is ignored during recovery. append_snapshot flushes the file to stable storage where
// the platform exposes fsync/_commit.
class RecoveryJournal {
public:
    explicit RecoveryJournal(std::filesystem::path path) : path_(std::move(path)) {}
    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }
    bool append_snapshot(const Project& project, Error& error) const;
    // Restores the newest complete, parseable snapshot. A damaged trailing record is
    // ignored; returns false if no valid snapshot exists.
    bool recover_latest(Project& out, Error& error) const;
private:
    std::filesystem::path path_;
};
}
