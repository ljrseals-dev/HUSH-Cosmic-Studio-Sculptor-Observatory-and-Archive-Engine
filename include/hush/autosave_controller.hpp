#pragma once
#include "hush/recovery_journal.hpp"
#include <chrono>
#include <optional>
#include <string>

namespace hush {
enum class SaveError { None, JournalWriteFailed, ProjectWriteFailed };

// Scheduling policy only. Call from a GUI/main or dedicated non-audio thread;
// never call save/poll from an audio callback.
class AutosaveController {
public:
    using Clock = std::chrono::steady_clock;
    enum class Outcome { NotDue, Saved, Failed };

    explicit AutosaveController(RecoveryJournal journal,
                                std::chrono::milliseconds interval = std::chrono::minutes(2));
    Outcome poll(const Project& project, Clock::time_point now,
                 bool transport_running, Error& error);
    Outcome poll_if_dirty(const Project& project, Clock::time_point now,
                          bool& dirty, bool transport_running, Error& error);
    Outcome save_now(const Project& project, Error& error);
    void report_project_write_failure(std::string message);
    void reset() noexcept;
    [[nodiscard]] std::chrono::milliseconds interval() const noexcept { return interval_; }
    [[nodiscard]] SaveError last_save_error() const noexcept { return last_save_error_; }
    [[nodiscard]] const std::string& last_save_message() const noexcept { return last_save_message_; }

private:
    Outcome write_snapshot(const Project& project, Clock::time_point now, Error& error);
    RecoveryJournal journal_;
    std::chrono::milliseconds interval_;
    std::optional<Clock::time_point> last_save_;
    bool transport_was_running_{false};
    SaveError last_save_error_{SaveError::None};
    std::string last_save_message_;
};
} // namespace hush