#include "hush/autosave_controller.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>

namespace {
bool writer_succeeds = true;
int writer_calls = 0;

bool injected_writer(const std::filesystem::path&, const hush::Project&, hush::Error& error) {
    ++writer_calls;
    if (!writer_succeeds) {
        error = {"injected_write_failure", "", "injected journal failure", "error"};
        return false;
    }
    error = {};
    return true;
}
} // namespace

int main() {
    using namespace hush;
    using Clock = AutosaveController::Clock;
    const auto epoch = Clock::time_point{};

    writer_calls = 0;
    writer_succeeds = true;
    AutosaveController controller(
        RecoveryJournal{std::filesystem::path{"unused-test-journal"}, &injected_writer},
        std::chrono::seconds{10});
    Project project;
    project.id = "autosave-test";
    project.tracks.push_back({"track-1", "Track 1", 0.75F, 0.0F, false, false});
    bool dirty = false;
    Error error;

    // A clean project must not write, even on the initial tick.
    assert(controller.poll_if_dirty(project, epoch, dirty, false, error) ==
           AutosaveController::Outcome::NotDue);
    assert(writer_calls == 0);

    // First dirty poll saves immediately and clears dirty only after success.
    dirty = true;
    assert(controller.poll_if_dirty(project, epoch, dirty, false, error) ==
           AutosaveController::Outcome::Saved);
    assert(!dirty && writer_calls == 1);
    assert(controller.last_save_error() == SaveError::None);

    // A newly dirty project waits for the deadline, unless transport just stopped.
    dirty = true;
    assert(controller.poll_if_dirty(project, epoch + std::chrono::seconds{3},
                                    dirty, true, error) == AutosaveController::Outcome::NotDue);
    assert(dirty && writer_calls == 1);
    assert(controller.poll_if_dirty(project, epoch + std::chrono::seconds{4},
                                    dirty, false, error) == AutosaveController::Outcome::Saved);
    assert(!dirty && writer_calls == 2);

    // Failed journal writes remain dirty and expose a typed error.
    dirty = true;
    writer_succeeds = false;
    assert(controller.poll_if_dirty(project, epoch + std::chrono::seconds{5},
                                    dirty, false, error) == AutosaveController::Outcome::NotDue);
    assert(dirty && writer_calls == 2);
    assert(controller.poll_if_dirty(project, epoch + std::chrono::seconds{14},
                                    dirty, false, error) == AutosaveController::Outcome::Failed);
    assert(dirty && writer_calls == 3);
    assert(controller.last_save_error() == SaveError::JournalWriteFailed);
    assert(controller.last_save_message() == "injected journal failure");

    // Explicit save and project-file errors update the same persistent status.
    writer_succeeds = true;
    assert(controller.save_now(project, error) == AutosaveController::Outcome::Saved);
    assert(controller.last_save_error() == SaveError::None);
    assert(controller.last_save_message().empty());
    controller.report_project_write_failure("disk full");
    assert(controller.last_save_error() == SaveError::ProjectWriteFailed);
    assert(controller.last_save_message() == "disk full");

    // Invalid intervals are clamped and clock rollback cannot trigger a busy save loop.
    AutosaveController rollback_controller(
        RecoveryJournal{std::filesystem::path{"unused-test-journal"}, &injected_writer},
        std::chrono::milliseconds{0});
    assert(rollback_controller.interval() == std::chrono::milliseconds{1});
    writer_calls = 0;
    dirty = true;
    assert(rollback_controller.poll_if_dirty(project, epoch + std::chrono::seconds{2},
                                             dirty, false, error) == AutosaveController::Outcome::Saved);
    dirty = true;
    assert(rollback_controller.poll_if_dirty(project, epoch + std::chrono::seconds{1},
                                             dirty, false, error) == AutosaveController::Outcome::NotDue);
    assert(dirty && writer_calls == 1);
    rollback_controller.reset();
    assert(rollback_controller.last_save_error() == SaveError::None);
    return 0;
}
