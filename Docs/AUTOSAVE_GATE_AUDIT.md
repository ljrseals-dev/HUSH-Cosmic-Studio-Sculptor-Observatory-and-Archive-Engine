# Autosave Gate 7 — lifecycle audit

Audit branch: `feat/autosave-lifecycle-gate7-main`, based directly on current `main`.

## 1. Policy and journal behavior

- `include/hush/autosave_controller.hpp` documents a policy object, not a timer or worker. Every call must be made on the GUI/main thread or a dedicated non-audio thread.
- The original `poll(project, now, transport_running, error)` saves on the first poll, at the interval, and on a running-to-stopped transition; it has no dirty input and writes a full-project JSON snapshot to the journal, not the selected project file.
- The Gate 7 branch adds `poll_if_dirty(snapshot, now, dirty, transport_running, error)`. A clean project is a no-op; a dirty project is saved on the first due poll, interval expiry, or stop transition. It adds `save_now`, `SaveError`, a last-error message, and an explicit project-file-write error setter. Successful journal saves clear the typed error and record the last-save time. Failed saves retain the prior successful timestamp, so a due save retries on a later poll; there is no exponential backoff.
- The default interval remains two minutes. The host owns the timer and passes a value-copy snapshot; no controller method is called from an audio callback.
- `RecoveryJournal::append_snapshot` serializes the full project, appends a length-framed record, flushes it, and requests `fsync`/`_commit`. Its errors are returned through `hush::Error`. The branch adds an optional `AppendWriter` function pointer, defaulting to the real filesystem writer, for deterministic failure injection.
- `RecoveryJournal::recover_if_different` applies the newest complete snapshot only when it differs from the current project, so hosts can decide whether a recovery notice is warranted.
- No power-loss durability claim is added. The README's existing statement about process-restart recovery and truncated-tail handling is preserved verbatim.

## 2. Lifecycle hook map

| Event | Gate 7 branch behavior | Remaining limit |
|---|---|---|
| Application start | GUI demo session does not create an autosave controller until a project is opened or saved. CLI supports `--project FILE` and optional `--journal FILE` before audio startup. | CLI host still has no project-edit command surface. |
| Project open | GUI parses JSON, checks the adjacent journal, applies a differing complete snapshot, persists recovered JSON to the canonical project file, then creates the controller and starts the main-thread timer. CLI does the corresponding load/recovery/persist sequence before starting ALSA. | Actual GUI and CLI notice behavior needs host-level tests. |
| Project open failure | GUI shows a blocking warning and does not replace the current project. CLI emits a single-line stderr diagnostic and exits 1. | Short-read GUI error handling is not separately tested. |
| Project close / replacement | GUI asks Save/Discard/Cancel when the operation log is dirty; Cancel or failed/cancelled Save blocks replacement. | Qt dialog test not yet implemented. |
| Mutation / undo / redo | OperationLog marks dirty on append, successful apply, undo, and redo. GUI refreshes a persistent dirty/error label after mutations. | Dirty state is conservative; undoing back to an earlier saved value still remains dirty. |
| Autosave tick | GUI QTimer runs on the main thread only after a project session is open. It copies the project, passes the copy to `poll_if_dirty`, and clears dirty only on `Saved`. CLI's main loop contains the same non-audio-thread policy call, but the host has no edit command so normal use does not make it dirty. | No dedicated autosave worker is used. |
| User save | GUI writes the project JSON, flushes/checks it, then synchronously appends a journal snapshot; dirty clears only after both succeed. Failure is shown in a modal warning and persistent status. | No Qt save-dialog automation yet. |
| Quit while dirty | GUI closeEvent asks Save/Discard/Cancel; failed or cancelled Save blocks close. CLI shutdown path returns 2 on final-save failure. | CLI dirty shutdown branch is not reachable through current host commands and is not tested. |
| Recovery on next start | A differing journal snapshot is applied and persisted to the canonical project path; CLI emits one stderr line with the journal path. GUI shows a status notice once for that open. | Existing unit test verifies the journal state comparison, not the actual GUI label or CLI stderr. |

## 3. Failure and silent-error audit

- `src/recovery_journal.cpp`: `catch (...)` while parsing a record header breaks journal scanning. This is intentionally tolerant of malformed/truncated tails, but it does not distinguish malformed input from an unexpected exception in the public error surface.
- `src/recovery_journal.cpp`: short header/payload/terminator reads stop scanning. If an earlier complete record exists, it is returned and the damaged tail is ignored; this behavior is covered by existing partial-write tests and is not evidence of power-loss durability.
- `src/recovery_journal.cpp`: directory creation, open, record write/flush, and sync failures are converted to `Error` and return false.
- `src/autosave_controller.cpp`: journal failure returns `Outcome::Failed` and stores a typed error/message. GUI exposes it in the persistent save-status label; CLI prints from its main/shutdown path, not the audio callback.
- `src/mixer_gui.cpp`: open failures and JSON parse failures are shown with QMessageBox. Project write open/short-write/flush failures remain dirty and are shown in both the status label and a blocking explicit-save warning. Journal failure on explicit save is also shown and blocks close.
- `src/alsa_realtime_host.cpp`: argument/project-open failures emit a single-line stderr message and return 1; recovery/project/journal/final-save failures return 2. Signal handler only flips a `sig_atomic_t`; file I/O and messages occur on the main loop/shutdown path.
- `src/operation_log.cpp`: apply/undo/redo return false when the target entity is missing; they do not throw. The branch adds dirty tracking to successful history/state changes.

## 4. Tests

The branch adds `tests/autosave_lifecycle_tests.cpp` for:
- clean tick with zero writer calls;
- due dirty tick with one journal record;
- injected journal failure retaining dirty and exposing `SaveError::JournalWriteFailed`;
- synchronous save success/failure and project-file error typing;
- snapshot-copy isolation with the writer running on a designated non-audio thread;
- dirty transitions for all current operation kinds (gain, pan, mute, solo);
- recovery of the newest complete snapshot with a truncated tail and no second state-change notice after the recovered state becomes canonical.

Existing `tests/recovery_restart_tests.cpp`, `tests/recovery_partial_write_tests.cpp`, and `tests/contract_tests.cpp` continue to cover process-restart recovery, truncated tails, and the original scheduling contract. The branch also registers `hush_alsa_host_help` so CLI help/exit code can be tested without opening an audio device.

Fresh CI passed all seven jobs in [run 39291973496](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39291973496), including default/ALSA/Qt+ALSA builds, ASan, TSan, and repository hygiene. Qt close-dialog behavior, actual GUI status-label behavior, and CLI dirty-quit failure remain open.

## 5. Risk-ordered residual gaps

1. Re-run default, ALSA, GUI+ALSA, ASan, TSan, and privacy-audit CI after any subsequent source changes; all passed in run 39291973496.
2. Qt headless test for save failure blocking close and the status/error surface.
3. CLI test for dirty final-save failure; the current host has no edit command to make that state reachable.
4. Host-level test proving a recovery notice is emitted once and not on a second start.
5. Named target-machine ALSA startup/shutdown, timing, xrun, loopback, and soak reports; these are outside a source-only implementation gate.

## Threading boundary

The GUI QTimer callback, project snapshot copy, project-file write, journal append, recovery parse/write, CLI diagnostics, and shutdown save all run on the GUI/main thread. ALSA's callback does not call autosave or journal code. Do not move journal I/O into the callback.
