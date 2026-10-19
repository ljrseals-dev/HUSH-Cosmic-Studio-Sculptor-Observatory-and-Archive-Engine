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
    // Optional deterministic test seam. A null writer selects the real filesystem
    // implementation. Injected writers are called only by explicit non-audio save paths.
    using AppendWriter = bool (*)(const std::filesystem::path&, const Project&, Error&);
    explicit RecoveryJournal(std::filesystem::path path, AppendWriter writer = nullptr)
        : path_(std::move(path)), append_writer_(writer) {}
    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }
    bool append_snapshot(const Project& project, Error& error) const;
    // Restores the newest complete, parseable snapshot. A damaged trailing record is
    // ignored; returns false if no valid snapshot exists.
    bool recover_latest(Project& out, Error& error) const;
    // Replaces in_out only when the newest complete journal snapshot differs.
    // applied is false when the current project already matches the journal.
    bool recover_if_different(Project& in_out, bool& applied, Error& error) const;
private:
    std::filesystem::path path_;
    AppendWriter append_writer_{nullptr};
};
}
