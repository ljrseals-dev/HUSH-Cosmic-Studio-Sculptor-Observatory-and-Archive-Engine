# Autosave Gate 7 — source audit

Audit target: `feat/autosave-lifecycle-gate7`, based on `feat/gui-live-mixer-wiring`.

## 1. Current policy behavior

- `include/hush/autosave_controller.hpp` documents that all save calls are scheduling/policy operations only and must be invoked by a GUI/main or dedicated non-audio thread. The controller owns no timer or worker thread.
- `src/autosave_controller.cpp` now exposes `poll_if_dirty(snapshot, now, dirty, transport_running, error)`. Clean state is a no-op; dirty state saves on the first due poll, interval expiry, or running-to-stopped transport transition. The original `poll` overload remains as a compatibility wrapper that treats its input as dirty.
- `save_now` appends a full project JSON snapshot to `RecoveryJournal`, clears the typed error on success, and updates the last-save time. It does not write the selected project JSON file; the GUI/CLI host must write that file separately. The controller stores `SaveError` plus a short message and supports `report_project_write_failure`.
- Default interval is two minutes. Failed writes do not advance the last successful save time, so a due save is retried on the next poll; there is no exponential backoff. The injected `RecoveryJournal::AppendWriter` seam is opt-in and defaults to the production filesystem writer.
- `src/recovery_journal.cpp:35-59` serializes JSON, creates parent directories, appends a length-framed `HUSHJ1` record, flushes the stream, then requests `fsync`/`_commit`. Errors are returned through `hush::Error`. This is not a power-loss durability claim; the README's existing limitation remains frozen.
- Existing policy coverage is in `tests/contract_tests.cpp:55-69`: first poll saves, early poll is not due, interval poll saves, stopped-after-running poll saves, and the newest journal snapshot is recoverable. It does not test dirty gating, injected write failures, host lifecycle, or user-visible error behavior.

## 2. Missing lifecycle integration

| Event | Current behavior | Hook needed |
|---|---|---|
| Application start / recovery check | GUI constructs a demo project in `src/mixer_gui.cpp` and checks the selected project's adjacent journal after open. ALSA host accepts `--project` / `--journal`, loads JSON, and checks recovery before starting audio. | Implemented in the review branch; recovery-notice-once test remains open. |
| Project open | `MixerWindow::openProject` parses JSON, checks the adjacent journal, creates the controller, and surfaces an applied recovery snapshot in the save-status label. The CLI does the same before audio start. | Source hook implemented; automated host lifecycle verification pending. |
| Project close / replace | GUI stops live ALSA before validating the candidate project (`src/mixer_gui.cpp:274-285)); no dirty guard or final save. | GUI project replacement/close flow. |
| Quit while dirty | `src/mixer_gui.cpp:254-258` only stops live audio in the destructor; no close-event dirty check. ALSA host exits after signal and backend stop (`src/alsa_realtime_host.cpp:68-79`). | GUI `closeEvent`; CLI shutdown path before successful return. |
| Crash recovery next start | Journal recovery exists in `src/recovery_journal.cpp:61-94`, but neither host invokes it or surfaces a notice. | GUI startup/open path and CLI startup path. |
| Dirty transitions | `OperationLog` now tracks dirty state on append, successful apply, undo, and redo, with explicit `mark_dirty` / `mark_saved`. The GUI uses this state for the status indicator and clears it only after a journal snapshot succeeds. The CLI host currently exposes no edit command. | GUI chokepoint implemented; CLI edit parity is not applicable until the host gains edits. |
| User-initiated save | GUI writes JSON, flushes/checks the file, writes a journal snapshot synchronously, then clears operation-log dirty state only after both succeed. | Source hook implemented; save-failure tests and fresh CI pending. |
| Live ALSA start/stop | `src/mixer_gui.cpp:374-394` starts/stops only the live monitor. The policy is not called. | Do not couple autosave to the audio callback; session timer stays on GUI/main thread and starts only after a project is loaded. |

## 3. Failure propagation / silent-failure audit

- `src/recovery_journal.cpp:76`: `catch (...)` exits record scanning. This is intentionally tolerant of malformed/truncated journal tails, but does not distinguish malformed data from an unexpected exception in its public error surface.
- `src/recovery_journal.cpp:64-85`: stream read/header/payload/terminator failures break scanning. If an earlier valid record exists, `recover_latest` returns it with a cleared error (`src/recovery_journal.cpp:91-93`); a damaged tail is therefore intentionally ignored without a recovery-warning flag.
- `src/recovery_journal.cpp:44-56`: directory creation, open, write/flush, and sync failures are converted to `Error` and return false. Callers must surface these errors.
- `src/autosave_controller.cpp`: failed append returns `Outcome::Failed`, preserves the last successful timestamp, and stores a typed error/message. The GUI displays a persistent status warning; the CLI emits a single-line stderr error from its main/shutdown path. Fresh CI is pending.
- `src/mixer_gui.cpp:279-288`: open failure and JSON parse failure are shown in QMessageBox; read completion/short-read status is not separately checked.
- `src/mixer_gui.cpp:299-306`: project save open/short-write failures are shown, but there is no common typed save error and no explicit flush/close-result check. The save does not write the recovery journal.
- `src/mixer_gui.cpp:218-241`: undo/redo updates the project and displays a queue-full status, but does not set a persistent dirty state.
- `src/alsa_realtime_host.cpp:68-79`: shutdown stops the backend and prints counters, but does not save/recover a project or return a save-specific exit code.
- `src/operation_log.cpp:4-6`: `append` truncates redo history and appends, but it has no dirty callback. `src/operation_log.cpp:20-31` returns false on missing entity but does not provide an error message.

## 4. GUI and host surfaces

- GUI now has a separate dirty/error label, an autosave timer that starts after project open/save, project-file flush checks, synchronous journal save on user save, a Save/Discard/Cancel close guard, and a recovery notice. A successfully applied recovery snapshot is written back to the canonical project file so the same recovered state is not announced again on the next start; if that write fails, the project remains dirty and the error is visible.
- GUI destructor still stops the live backend; the closeEvent guard is the intended quit path. Automated Qt close-dialog coverage is not yet implemented.
- ALSA host now accepts `--project` and optional `--journal`, loads a project before starting audio, applies a changed journal snapshot, persists it, emits `HUSH_RECOVERY_APPLIED path=...` once for that state change, and documents exit codes in `--help`. Its current host has no project-edit command surface, so ordinary operation cannot make its project dirty; CLI dirty-edit/final-save failure behavior remains untested.
- Existing GUI status polling reports backend callback and xrun/recovery counters only (`src/mixer_gui.cpp:352-371`).

## 5. Existing tests

- `tests/contract_tests.cpp:55-69` tests the scheduler policy with a temporary journal and simulated steady-clock timestamps.
- `tests/recovery_restart_tests.cpp` tests completed journal-record recovery after a child process exits and restarts.
- `tests/recovery_partial_write_tests.cpp` (registered in CMake) covers interrupted headers and truncated payloads.
- `tests/autosave_lifecycle_tests.cpp` now covers clean ticks, due journal writes, injected writer failure and retained dirty state, synchronous save success/failure, typed error state, snapshot-copy isolation, a writer-thread check against a designated test audio thread, operation-kind dirty transitions, and changed-snapshot recovery being applied only once after the canonical state is updated. These tests have not yet received fresh CI confirmation. Qt close-dialog behavior and CLI dirty-quit failure are not yet asserted.

## 6. Risk-ordered gap list

1. **Data loss on replacement/quit:** add dirty tracking and close/quit save guards in `src/mixer_gui.cpp`; the ALSA host needs a project/session lifecycle before it can offer equivalent behavior.
2. **Failed save can be mistaken for clean state:** add a typed error and retain dirty state in the controller/session API; surface it in GUI and CLI.
3. **No host invokes autosave:** add a GUI-thread timer around a controller that snapshots a project copy; never call it from ALSA callbacks.
4. **No failure injection:** add an injectable journal writer seam and deterministic tests for open/write/sync failures.
5. **Recovery is silent:** report whether tail damage/recovery was applied without changing the README's power-loss statement; surface once per startup.
6. **Operation-log bypass:** centralize dirty-state changes around successful apply/undo/redo and make the dirty state testable.
7. **No command-line session surface:** `src/alsa_realtime_host.cpp` currently takes no arguments and creates a hard-coded graph. CLI project open/save/recovery requires an explicit scope change to its current demo-host behavior.
8. **Baseline evidence gap:** GitHub Actions has passed the default, ALSA, and Qt+ALSA build/test jobs on run 39255426377, but no local terminal transcript is available to record verbatim from this execution environment. Do not fabricate shell output.

## Gate 7 implementation delta and remaining limits

The review branch now adds `SaveError`, `poll_if_dirty`, `save_now`, an injectable `RecoveryJournal::AppendWriter`, operation-log dirty tracking, `tests/autosave_lifecycle_tests.cpp`, GUI main-thread timer/snapshot/save/quit hooks, and CLI `--project` / `--journal` / `--help` handling. The CLI host still has no project-edit command surface, so its dirty-edit/final-save failure branch is defensive but not exercised by normal host operation. The lifecycle tests exercise recovery-state comparison and repeated-start behavior at the journal API level, but do not yet test the actual GUI status label or CLI stderr line. Fresh CI for the review branch has not reported at the time of this audit.

## Threading boundary

All project snapshots, journal writes, save dialogs, dirty-state changes, and recovery notices must run on the GUI/main thread or a dedicated non-audio worker. `AutosaveController::poll` is explicitly documented as non-audio-only. ALSA callbacks must never call it or perform project/journal file I/O.
