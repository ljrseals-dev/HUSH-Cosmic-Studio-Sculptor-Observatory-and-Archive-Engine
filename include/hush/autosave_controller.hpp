#pragma once
#include "hush/recovery_journal.hpp"
#include <chrono>
#include <optional>
#include <string>

namespace hush {

enum class SaveError {
    None,
    JournalWriteFailed,
    ProjectWriteFailed,
    DiskFull,
    PermissionDenied,
    Unknown
};

// Policy only: every method must be called on a GUI/main or dedicated non-audio
// thread. The host owns the timer/thread and passes a value-copy project snapshot.
class AutosaveController {
public:
    using Clock = std::chrono::steady_clock;
    enum class Outcome { NotDue, Saved, Failed };

    explicit AutosaveController(RecoveryJournal journal,
                                std::chrono::milliseconds interval = std::chrono::minutes(2));

    Outcome poll_if_dirty(const Project& snapshot, Clock::time_point now, bool dirty,
                          bool transport_running, Error& error);
    Outcome save_now(const Project& snapshot, Error& error);

    [[nodiscard]] SaveError last_save_error() const noexcept { return last_save_error_; }
    [[nodiscard]] const std::string& last_save_message() const noexcept { return last_save_message_; }
    [[nodiscard]] std::chrono::milliseconds interval() const noexcept { return interval_; }
    void report_project_write_failure(std::string message);
    void reset() noexcept;

    // Compatibility for the original policy API; new host code must pass dirty state.
    Outcome poll(const Project& project, Clock::time_point now,
                 bool transport_running, Error& error) {
        return poll_if_dirty(project, now, true, transport_running, error);
    }

private:
    RecoveryJournal journal_;
    std::chrono::milliseconds interval_;
    std::optional<Clock::time_point> last_save_;
    bool transport_was_running_{false};
    SaveError last_save_error_{SaveError::None};
    std::string last_save_message_;

    void record_error(const Error& error);
    void clear_error() noexcept;
};

} // namespace hush
