#include "hush/autosave_controller.hpp"
#include "hush/operation_log.hpp"

#include <atomic>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <thread>

namespace {
std::atomic<int> append_calls{0};
std::atomic<bool> inject_failure{false};
std::thread::id expected_writer_thread{};
std::thread::id forbidden_audio_thread{};
std::atomic<bool> writer_on_expected_thread{false};
std::atomic<bool> writer_on_audio_thread{false};

bool injected_writer(const std::filesystem::path& path, const hush::Project& project,
                     hush::Error& error) {
    append_calls.fetch_add(1, std::memory_order_relaxed);
    writer_on_expected_thread.store(std::this_thread::get_id() == expected_writer_thread,
                                    std::memory_order_relaxed);
    writer_on_audio_thread.store(std::this_thread::get_id() == forbidden_audio_thread,
                                 std::memory_order_relaxed);
    if (inject_failure.load(std::memory_order_relaxed)) {
        error = {"journal_write_failed", project.id, "injected journal write failure", "error"};
        return false;
    }
    return hush::RecoveryJournal(path).append_snapshot(project, error);
}
} // namespace

int main() {
    using namespace hush;
    const auto path = std::filesystem::temp_directory_path() / "hush-autosave-lifecycle-tests.journal";
    std::error_code cleanup;
    std::filesystem::remove(path, cleanup);
    append_calls.store(0);
    inject_failure.store(false);

    Project project;
    project.id = "lifecycle";
    project.tracks.push_back({"track-1", "Lead", 1.0F, 0.0F, false, false});
    AutosaveController controller(RecoveryJournal(path, &injected_writer),
                                  std::chrono::milliseconds(100));
    Error error;
    const auto t0 = AutosaveController::Clock::time_point{};

    // Clean tick is a no-op: no writer call and clean state remains clean.
    bool dirty = false;
    assert(controller.poll_if_dirty(project, t0, dirty, false, error) ==
           AutosaveController::Outcome::NotDue);
    assert(!dirty && append_calls.load() == 0);

    // A dirty, due tick writes exactly one journal record. Host clears dirty only on Saved.
    dirty = true;
    assert(controller.poll_if_dirty(project, t0, dirty, false, error) ==
           AutosaveController::Outcome::Saved);
    if (controller.last_save_error() == SaveError::None) dirty = false;
    assert(!dirty && append_calls.load() == 1);
    assert(controller.last_save_error() == SaveError::None);
    assert(RecoveryJournal(path).recover_latest(project, error));

    // Recovery applies the newest complete snapshot once; after the host persists
    // that state as its canonical project, the next start does not re-announce it.
    Project on_disk;
    on_disk.id = "older-disk-state";
    Project recovered_state;
    recovered_state.id = "newer-recovery-state";
    recovered_state.tracks.push_back({"recover-track", "Recovered", 0.8F, 0.1F, false, false});
    const auto recovery_path = std::filesystem::temp_directory_path() / "hush-autosave-recovery-notice.journal";
    std::filesystem::remove(recovery_path, cleanup);
    assert(RecoveryJournal(recovery_path).append_snapshot(recovered_state, error));
    { std::ofstream tail(recovery_path, std::ios::binary | std::ios::app); tail << "HUSHJ1 99\\npartial"; }
    bool recovery_applied = false;
    assert(RecoveryJournal(recovery_path).recover_if_different(on_disk, recovery_applied, error));
    assert(recovery_applied && on_disk == recovered_state);
    // Simulate the host persisting the recovered state before exiting; on next start
    // the same journal is no longer a state change and must not trigger a notice.
    recovery_applied = true;
    assert(RecoveryJournal(recovery_path).recover_if_different(on_disk, recovery_applied, error));
    assert(!recovery_applied && on_disk == recovered_state);
    std::filesystem::remove(recovery_path, cleanup);

    // Dirty changes before the interval do not cause excess writes.
    dirty = true;
    assert(controller.poll_if_dirty(project, t0 + std::chrono::milliseconds(50),
                                    dirty, false, error) == AutosaveController::Outcome::NotDue);
    assert(dirty && append_calls.load() == 1);

    // Injected failure must be returned as state, not thrown, and must not clear dirty.
    inject_failure.store(true);
    assert(controller.poll_if_dirty(project, t0 + std::chrono::milliseconds(100),
                                    dirty, false, error) == AutosaveController::Outcome::Failed);
    assert(dirty);
    assert(controller.last_save_error() == SaveError::JournalWriteFailed);
    assert(controller.last_save_message() == "injected journal write failure");
    controller.report_project_write_failure("injected project file failure");
    assert(controller.last_save_error() == SaveError::ProjectWriteFailed);
    assert(controller.last_save_message() == "injected project file failure");

    // Explicit synchronous save follows the same journal path and error contract.
    inject_failure.store(false);
    assert(controller.save_now(project, error) == AutosaveController::Outcome::Saved);
    dirty = false;
    assert(!dirty && controller.last_save_error() == SaveError::None);
    inject_failure.store(true);
    dirty = true;
    assert(controller.save_now(project, error) == AutosaveController::Outcome::Failed);
    assert(dirty && controller.last_save_error() == SaveError::JournalWriteFailed);

    // Snapshot isolation: the save thread receives a value-copy made on the caller
    // thread; the caller can continue editing the original project without a race.
    inject_failure.store(false);
    Project snapshot = project;
    expected_writer_thread = std::thread::id{};
    std::atomic<bool> audio_thread_ready{false};
    std::atomic<bool> release_audio_thread{false};
    std::thread fake_audio_thread([&] {
        forbidden_audio_thread = std::this_thread::get_id();
        audio_thread_ready.store(true, std::memory_order_release);
        while (!release_audio_thread.load(std::memory_order_acquire)) std::this_thread::yield();
    });
    while (!audio_thread_ready.load(std::memory_order_acquire)) std::this_thread::yield();
    std::atomic<bool> save_ok{false};
    std::thread save_thread([&] {
        expected_writer_thread = std::this_thread::get_id();
        Error thread_error;
        save_ok.store(controller.save_now(snapshot, thread_error) ==
                      AutosaveController::Outcome::Saved, std::memory_order_relaxed);
    });
    project.tracks.front().gain = 0.25F;
    save_thread.join();
    assert(save_ok.load(std::memory_order_relaxed));
    assert(writer_on_expected_thread.load(std::memory_order_relaxed));
    assert(!writer_on_audio_thread.load(std::memory_order_relaxed));
    release_audio_thread.store(true, std::memory_order_release);
    fake_audio_thread.join();

    // Every current operation kind marks dirty on apply, undo, and redo.
    const OperationKind kinds[] = {OperationKind::SetGain, OperationKind::SetPan,
                                   OperationKind::SetMute, OperationKind::SetSolo};
    for (const auto kind : kinds) {
        Project p;
        p.tracks.push_back({"t", "Track", 1.0F, 0.0F, false, false});
        OperationLog log;
        assert(!log.dirty());
        const float before = (kind == OperationKind::SetGain) ? 1.0F : 0.0F;
        const float after = (kind == OperationKind::SetGain) ? 0.5F : 1.0F;
        assert(log.apply_and_append(p, {"t", kind, before, after}));
        assert(log.dirty());
        log.mark_saved();
        assert(!log.dirty());
        assert(log.undo(p) && log.dirty());
        log.mark_saved();
        assert(!log.dirty());
        assert(log.redo(p) && log.dirty());
    }

    std::filesystem::remove(path, cleanup);
    return 0;
}
