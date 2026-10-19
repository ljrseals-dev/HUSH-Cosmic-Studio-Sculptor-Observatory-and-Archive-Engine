#pragma once
#include "hush/recovery_journal.hpp"
#include <chrono>
#include <optional>
#include <string>

namespace hush {

// Typed, user-presentable save outcome. The journal still reports detailed Error
// codes/messages; this enum gives hosts a stable surface for status and exit policy.
enum class SaveError {
    None,
    JournalWriteFailed,
    ProjectWriteFailed,
    DiskFull,
    PermissionDenied,
    Unknown
};

// Scheduling policy only: the host application must call poll_if_dirty()/save_now()
// from a GUI/main or dedicated non-audio thread. This class owns no timer/thread.
// Never call any save method from an audio callback.
class AutosaveController {
public:
    using Clock = std::chrono::steady_clock;
    enum class Outcome { NotDue, Saved, Failed };

    explicit AutosaveController(RecoveryJournal journal,
                                std::chrono::milliseconds interval = std::chrono::minutes(2));

    // Preferred lifecycle API: clean projects do not write. Dirty state is owned by
    // the host and may be cleared only after Outcome::Saved.
    Outcome poll_if_dirty(const Project& snapshot, Clock::time_point now,
                          bool dirty, bool transport_running, Error& error);

    // Synchronous journal snapshot for explicit save / shutdown paths. This does not
    // save the user-selected project JSON file; hosts must perform that write too.
    Outcome save_now(const Project& snapshot, Error& error);

    [[nodiscard]] SaveError last_save_error() const noexcept { return last_save_error_; }
    [[nodiscard]] const std::string& last_save_message() const noexcept { return last_save_message_; }
    void report_project_write_failure(std::string message);
    [[nodiscard]] std::chrono::milliseconds interval() const noexcept { return interval_; }
    void reset() noexcept;

    // Compatibility wrapper for the original policy tests/API. Treats the supplied
    // project as dirty; new host code must use poll_if_dirty().
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
