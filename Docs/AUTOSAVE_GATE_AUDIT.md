# Autosave Gate 7 — source audit

Audit target: `feat/autosave-lifecycle-gate7`, based on `feat/gui-live-mixer-wiring`.

## 1. Current policy behavior

- `include/hush/autosave_controller.hpp:8-10` states this is scheduling policy only; the host must call `poll()` from a non-audio thread. The controller owns neither timer nor worker thread.
- `src/autosave_controller.cpp:14-17` computes an interval trigger and a running-to-stopped transport trigger. There is no dirty-state input. `src/autosave_controller.cpp:15-16` makes the first poll due because `last_save_` is empty; a backwards clock value also forces a save.
- `src/autosave_controller.cpp:22-23` calls `RecoveryJournal::append_snapshot`; it appends a full project JSON snapshot to the journal, not the selected project file. The controller records `last_save_` only after success. It has no typed save error, user-facing message, dirty state, or retry/backoff state.
- Default interval is two minutes (`include/hush/autosave_controller.hpp:16-17`). Failed writes do not advance `last_save_`, so a due save is retried on the next poll; there is no exponential backoff.
- `src/recovery_journal.cpp:35-59` serializes JSON, creates parent directories, appends a length-framed `HUSHJ1` record, flushes the stream, then requests `fsync`/`_commit`. Errors are returned through `hush::Error`. This is not a power-loss durability claim; the README's existing limitation remains frozen.
- Existing policy coverage is in `tests/contract_tests.cpp:55-69`: first poll saves, early poll is not due, interval poll saves, stopped-after-running poll saves, and the newest journal snapshot is recoverable. It does not test dirty gating, injected write failures, host lifecycle, or user-visible error behavior.

## 2. Missing lifecycle integration

| Event | Current behavior | Hook needed |
|---|---|---|
| Application start / recovery check | GUI constructs a demo project in `src/mixer_gui.cpp:246-252`; no recovery journal is checked. ALSA host builds a fixed graph in `src/alsa_realtime_host.cpp:25-47`. | GUI startup/project-session controller; CLI host needs a project/session argument and recovery policy before audio starts. |
| Project open | `src/mixer_gui.cpp:271-293` opens JSON and replaces project state; no autosave controller starts and no journal recovery notice appears. | `MixerWindow::openProject` after successful parse. |
| Project close / replace | GUI stops live ALSA before validating the candidate project (`src/mixer_gui.cpp:274-285)); no dirty guard or final save. | GUI project replacement/close flow. |
| Quit while dirty | `src/mixer_gui.cpp:254-258` only stops live audio in the destructor; no close-event dirty check. ALSA host exits after signal and backend stop (`src/alsa_realtime_host.cpp:68-79`). | GUI `closeEvent`; CLI shutdown path before successful return. |
| Crash recovery next start | Journal recovery exists in `src/recovery_journal.cpp:61-94`, but neither host invokes it or surfaces a notice. | GUI startup/open path and CLI startup path. |
| Dirty transitions | `src/mixer_gui.cpp:319-330` mutates through `OperationLog::apply_and_append`; undo/redo at lines 218-241 mutate project. No dirty bit exists. `OperationLog::append` at `src/operation_log.cpp:4-6` can append history without a project/dirty callback. | One explicit dirty-state chokepoint around successful apply, undo, redo, and project replacement/save baseline. |
| User-initiated save | `src/mixer_gui.cpp:295-309` writes JSON to a path selected with QFileDialog. It does not update a canonical project path, journal snapshot, or dirty state. | GUI save path and shared save operation. |
| Live ALSA start/stop | `src/mixer_gui.cpp:374-394` starts/stops only the live monitor. The policy is not called. | Do not couple autosave to the audio callback; session timer stays on GUI/main thread and starts only after a project is loaded. |

## 3. Failure propagation / silent-failure audit

- `src/recovery_journal.cpp:76`: `catch (...)` exits record scanning. This is intentionally tolerant of malformed/truncated journal tails, but does not distinguish malformed data from an unexpected exception in its public error surface.
- `src/recovery_journal.cpp:64-85`: stream read/header/payload/terminator failures break scanning. If an earlier valid record exists, `recover_latest` returns it with a cleared error (`src/recovery_journal.cpp:91-93`); a damaged tail is therefore intentionally ignored without a recovery-warning flag.
- `src/recovery_journal.cpp:44-56`: directory creation, open, write/flush, and sync failures are converted to `Error` and return false. Callers must surface these errors.
- `src/autosave_controller.cpp:22`: failed append returns `Outcome::Failed`; this policy itself does not log or display the error, and no host currently calls it.
- `src/mixer_gui.cpp:279-288`: open failure and JSON parse failure are shown in QMessageBox; read completion/short-read status is not separately checked.
- `src/mixer_gui.cpp:299-306`: project save open/short-write failures are shown, but there is no common typed save error and no explicit flush/close-result check. The save does not write the recovery journal.
- `src/mixer_gui.cpp:218-241`: undo/redo updates the project and displays a queue-full status, but does not set a persistent dirty state.
- `src/alsa_realtime_host.cpp:68-79`: shutdown stops the backend and prints counters, but does not save/recover a project or return a save-specific exit code.
- `src/operation_log.cpp:4-6`: `append` truncates redo history and appends, but it has no dirty callback. `src/operation_log.cpp:20-31` returns false on missing entity but does not provide an error message.

## 4. GUI and host surfaces

- GUI opens and saves JSON through QFileDialog and has open/save error dialogs, but has no dirty indicator beyond transient strings such as “Demo session · unsaved · offline” (`src/mixer_gui.cpp:251-252`). It has no journal error surface, recovery notice, or close confirmation.
- GUI destructor stops the live backend but does not attempt a final save (`src/mixer_gui.cpp:254-258`).
- ALSA host has no project file, operation log, autosave controller, or dirty state. It reports graph/device startup failures to stderr and prints callback/recovery counts after stopping; it has no save failure path or recovery notice.
- Existing GUI status polling reports backend callback and xrun/recovery counters only (`src/mixer_gui.cpp:352-371`).

## 5. Existing tests

- `tests/contract_tests.cpp:55-69` tests the scheduler policy with a temporary journal and simulated steady-clock timestamps.
- `tests/recovery_restart_tests.cpp` tests completed journal-record recovery after a child process exits and restarts.
- `tests/recovery_partial_write_tests.cpp` (registered in CMake) covers interrupted headers and truncated payloads.
- None of these currently asserts dirty-state lifecycle integration, typed save failures, quit handling, or a one-time recovery notice.

## 6. Risk-ordered gap list

1. **Data loss on replacement/quit:** add dirty tracking and close/quit save guards in `src/mixer_gui.cpp`; the ALSA host needs a project/session lifecycle before it can offer equivalent behavior.
2. **Failed save can be mistaken for clean state:** add a typed error and retain dirty state in the controller/session API; surface it in GUI and CLI.
3. **No host invokes autosave:** add a GUI-thread timer around a controller that snapshots a project copy; never call it from ALSA callbacks.
4. **No failure injection:** add an injectable journal writer seam and deterministic tests for open/write/sync failures.
5. **Recovery is silent:** report whether tail damage/recovery was applied without changing the README's power-loss statement; surface once per startup.
6. **Operation-log bypass:** centralize dirty-state changes around successful apply/undo/redo and make the dirty state testable.
7. **No command-line session surface:** `src/alsa_realtime_host.cpp` currently takes no arguments and creates a hard-coded graph. CLI project open/save/recovery requires an explicit scope change to its current demo-host behavior.
8. **Baseline evidence gap:** GitHub Actions has passed the default, ALSA, and Qt+ALSA build/test jobs on run 39255426377, but no local terminal transcript is available to record verbatim from this execution environment. Do not fabricate shell output.

## Threading boundary

All project snapshots, journal writes, save dialogs, dirty-state changes, and recovery notices must run on the GUI/main thread or a dedicated non-audio worker. `AutosaveController::poll` is explicitly documented as non-audio-only. ALSA callbacks must never call it or perform project/journal file I/O.
