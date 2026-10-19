#pragma once
#include "hush/recovery_journal.hpp"
#include <chrono>
#include <optional>

namespace hush {

// Scheduling policy only: the host application must call poll() regularly from a
// non-audio thread. This class owns no timer/thread and is safe to integrate with
// different event loops. Never call poll() from an audio callback.
class AutosaveController {
public:
    using Clock = std::chrono::steady_clock;
    enum class Outcome { NotDue, Saved, Failed };

    explicit AutosaveController(RecoveryJournal journal,
                                std::chrono::milliseconds interval = std::chrono::minutes(2));

    // Saves on the first poll, whenever interval elapses, and on a running->stopped
    // transport transition. transport_running must reflect the host's current state.
    Outcome poll(const Project& project, Clock::time_point now,
                 bool transport_running, Error& error);

    void reset() noexcept;
    [[nodiscard]] std::chrono::milliseconds interval() const noexcept { return interval_; }

private:
    RecoveryJournal journal_;
    std::chrono::milliseconds interval_;
    std::optional<Clock::time_point> last_save_;
    bool transport_was_running_{false};
};

} // namespace hush
