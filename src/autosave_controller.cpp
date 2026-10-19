#include "hush/autosave_controller.hpp"

#include <utility>

namespace hush {

AutosaveController::AutosaveController(RecoveryJournal journal,
                                       std::chrono::milliseconds interval)
    : journal_(std::move(journal)),
      interval_(interval.count() > 0 ? interval : std::chrono::milliseconds(1)) {}

void AutosaveController::record_error(const Error& error) {
    last_save_message_ = error.message.empty() ? "Save failed." : error.message;
    if (error.code.find("permission") != std::string::npos ||
        error.message.find("Permission denied") != std::string::npos ||
        error.message.find("permission denied") != std::string::npos) {
        last_save_error_ = SaveError::PermissionDenied;
    } else if (error.message.find("No space left") != std::string::npos ||
               error.message.find("no space left") != std::string::npos ||
               error.message.find("Disk quota exceeded") != std::string::npos) {
        last_save_error_ = SaveError::DiskFull;
    } else if (error.code.rfind("journal_", 0) == 0) {
        last_save_error_ = SaveError::JournalWriteFailed;
    } else {
        last_save_error_ = SaveError::Unknown;
    }
}

void AutosaveController::clear_error() noexcept {
    last_save_error_ = SaveError::None;
    last_save_message_.clear();
}

AutosaveController::Outcome AutosaveController::save_now(const Project& snapshot,
                                                          Error& error) {
    if (!journal_.append_snapshot(snapshot, error)) {
        record_error(error);
        return Outcome::Failed;
    }
    clear_error();
    last_save_ = Clock::now();
    return Outcome::Saved;
}

AutosaveController::Outcome AutosaveController::poll_if_dirty(
    const Project& snapshot, Clock::time_point now, bool dirty,
    bool transport_running, Error& error) {
    const bool stopped_after_running = transport_was_running_ && !transport_running;
    const bool interval_elapsed = !last_save_ || now < *last_save_ ||
                                  now - *last_save_ >= interval_;
    transport_was_running_ = transport_running;

    if (!dirty || (!stopped_after_running && !interval_elapsed)) {
        error = {};
        return Outcome::NotDue;
    }

    const auto outcome = save_now(snapshot, error);
    if (outcome == Outcome::Saved) last_save_ = now;
    return outcome;
}

void AutosaveController::reset() noexcept {
    last_save_.reset();
    transport_was_running_ = false;
    clear_error();
}

} // namespace hush
