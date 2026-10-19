#include "hush/autosave_controller.hpp"

namespace hush {

AutosaveController::AutosaveController(RecoveryJournal journal,
                                       std::chrono::milliseconds interval)
    : journal_(std::move(journal)),
      interval_(interval.count() > 0 ? interval : std::chrono::milliseconds(1)) {}

AutosaveController::Outcome AutosaveController::poll(const Project& project,
                                                      Clock::time_point now,
                                                      bool transport_running,
                                                      Error& error) {
    const bool stopped_after_running = transport_was_running_ && !transport_running;
    const bool interval_elapsed = !last_save_ || now < *last_save_ ||
                                  now - *last_save_ >= interval_;
    transport_was_running_ = transport_running;
    if (!stopped_after_running && !interval_elapsed) {
        error = {};
        return Outcome::NotDue;
    }
    if (!journal_.append_snapshot(project, error)) return Outcome::Failed;
    last_save_ = now;
    return Outcome::Saved;
}

void AutosaveController::reset() noexcept {
    last_save_.reset();
    transport_was_running_ = false;
}

} // namespace hush
