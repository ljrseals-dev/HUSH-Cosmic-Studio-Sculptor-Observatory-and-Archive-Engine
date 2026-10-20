#include "hush/autosave_controller.hpp"

#include <utility>

namespace hush {

AutosaveController::AutosaveController(RecoveryJournal journal,
                                       std::chrono::milliseconds interval)
    : journal_(std::move(journal)),
      interval_(interval.count() > 0 ? interval : std::chrono::milliseconds{1}) {}

AutosaveController::Outcome AutosaveController::write_snapshot(
    const Project& project, Clock::time_point now, Error& error) {
    if (!journal_.append_snapshot(project, error)) {
        last_save_error_ = SaveError::JournalWriteFailed;
        last_save_message_ = error.message.empty()
            ? "Could not append project snapshot to the recovery journal"
            : error.message;
        return Outcome::Failed;
    }

    last_save_ = now;
    last_save_error_ = SaveError::None;
    last_save_message_.clear();
    error = {};
    return Outcome::Saved;
}

AutosaveController::Outcome AutosaveController::poll(
    const Project& project, Clock::time_point now,
    bool transport_running, Error& error) {
    const bool stopped_after_running = transport_was_running_ && !transport_running;
    const bool interval_elapsed = !last_save_ ||
        (now >= *last_save_ && now - *last_save_ >= interval_);
    transport_was_running_ = transport_running;

    if (!stopped_after_running && !interval_elapsed) {
        error = {};
        return Outcome::NotDue;
    }
    return write_snapshot(project, now, error);
}

AutosaveController::Outcome AutosaveController::poll_if_dirty(
    const Project& project, Clock::time_point now, bool& dirty,
    bool transport_running, Error& error) {
    const bool stopped_after_running = transport_was_running_ && !transport_running;
    transport_was_running_ = transport_running;

    if (!dirty) {
        error = {};
        return Outcome::NotDue;
    }

    // A monotonic-clock rollback must not make every subsequent poll due. The
    // next save is eligible when time catches up and the normal interval expires.
    const bool interval_elapsed = !last_save_ ||
        (now >= *last_save_ && now - *last_save_ >= interval_);
    if (!stopped_after_running && !interval_elapsed) {
        error = {};
        return Outcome::NotDue;
    }

    const Outcome outcome = write_snapshot(project, now, error);
    if (outcome == Outcome::Saved) dirty = false;
    return outcome;
}

AutosaveController::Outcome AutosaveController::save_now(
    const Project& project, Error& error) {
    return write_snapshot(project, Clock::now(), error);
}

void AutosaveController::report_project_write_failure(std::string message) {
    last_save_error_ = SaveError::ProjectWriteFailed;
    last_save_message_ = std::move(message);
}

void AutosaveController::reset() noexcept {
    last_save_.reset();
    transport_was_running_ = false;
    last_save_error_ = SaveError::None;
    last_save_message_.clear();
}

} // namespace hush
