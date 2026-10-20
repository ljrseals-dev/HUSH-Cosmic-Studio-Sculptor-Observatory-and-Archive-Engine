#pragma once
#include "hush/project.hpp"
#include <filesystem>
#include <string>
#include <utility>

namespace hush {
// Append-only local recovery journal. A damaged trailing record is ignored on recovery.
class RecoveryJournal {
public:
    using AppendWriter = bool (*)(const std::filesystem::path&, const Project&, Error&);
    explicit RecoveryJournal(std::filesystem::path path, AppendWriter writer = nullptr)
        : path_(std::move(path)), append_writer_(writer) {}
    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }
    bool append_snapshot(const Project& project, Error& error) const;
    bool recover_latest(Project& out, Error& error) const;
    // Applies the newest valid snapshot only when it differs from the current project.
    bool recover_if_different(Project& in_out, bool& applied, Error& error) const;
private:
    std::filesystem::path path_;
    AppendWriter append_writer_{nullptr};
};
}