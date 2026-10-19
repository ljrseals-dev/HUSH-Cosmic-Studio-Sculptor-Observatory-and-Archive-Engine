# GUI interaction test audit — Gate 1 residual

## Scope and exact README gate statement

This audit reflects `src/mixer_gui.cpp`, `include/hush/realtime_track_mixer.hpp`, `src/realtime_track_mixer.cpp`, `tests/gui_engine_command_path_tests.cpp`, `tests/realtime_track_mixer_tests.cpp`, and `CMakeLists.txt` on this branch.

The README bullet being closed is quoted verbatim:

> **GUI-to-live-engine automated interaction test and GUI/CLI parity:** the optional Qt + ALSA path now queues GUI edits and undo/redo to the live callback; automated Qt interaction/loopback coverage and target-device validation remain outstanding.

## Interactive controls and evidence

| Control | GUI behavior | Queue command when live ALSA is running? | Test evidence |
|---|---|---|---|
| Gain slider drag | Enqueues a preview snapshot from `valueChanged`; release commits one undoable `SetGain` operation and enqueues the committed track snapshot | Yes, preview and committed snapshot | `tests/gui_engine_command_path_tests.cpp` covers preview/commit through production `OperationLog` and `RealtimeTrackMixer` APIs; `tests/gui_interaction_tests.cpp` exercises the actual Qt slider signal and project/undo history. CI result for this branch is pending. |
| Pan slider drag | Enqueues a preview snapshot from `valueChanged`; release commits one undoable `SetPan` operation | Yes, preview and committed snapshot | Same two tests as gain; CI result pending. |
| Mute checkbox | Commits `SetMute` on click | Yes, one full track-control snapshot | The command-path test covers mute processing; the Qt interaction test clicks the real checkbox and checks project/history. CI result pending. |
| Solo checkbox | Commits `SetSolo` on click | Yes, one full track-control snapshot | The command-path test covers solo processing; the Qt interaction test clicks the real checkbox and checks project/history. CI result pending. |
| Undo | Applies the previous operation to project state and submits the resulting track snapshot | Yes, if a live engine is running and a matching operation exists | Both command-path and Qt interaction tests cover undo; the Qt test checks the actual button and state. CI result pending. |
| Redo | Applies the next operation and submits the resulting track snapshot | Yes, if live and a matching operation exists | Both command-path and Qt interaction tests cover redo; the Qt test checks the actual button and state. CI result pending. |
| Start/Stop Live ALSA | Starts or stops the ALSA backend; no audio capture starts automatically | No UI edit command is emitted; startup seeds initial per-track snapshots before the callback starts | New Qt test checks the button is disabled in ALSA-off builds and enabled in ALSA-enabled builds. It deliberately does not click-start a physical device. CI result pending. |
| Open Project | Opens a user-selected local JSON project and replaces the in-memory project | No; live audio is stopped before loading | No Qt interaction test currently covers the file dialog. This is a local file action, not a realtime command. |
| Save Project | Writes project JSON to a user-selected local path | No; no audio-thread action | No Qt interaction test currently covers the file dialog. This is a local file action, not a realtime command. |

The Qt test uses the actual `MixerWindow` implementation by compiling `src/mixer_gui.cpp` with `HUSH_GUI_TESTING` and the real widgets. Test-only accessors are compiled only for that test target. Stable object names are used for widget discovery; they do not change production behavior.

## Queue command schema

The only track-mixer queue payload is `hush::TrackControlCommand` in `include/hush/realtime_track_mixer.hpp`:

- `track_index` (`std::uint32_t`): index into the preconfigured live track array.
- `gain` (`float`): validated range 0.0–2.0.
- `pan` (`float`): validated range -1.0–1.0.
- `flags` (`std::uint8_t`): bit 0 = mute; bit 1 = solo.

The GUI submits complete snapshots through `RealtimeTrackMixer::enqueue_control(track_index, gain, pan, mute, solo)`. The SPSC queue is bounded (capacity 2048); enqueue returns false on rejection/full queue. The audio thread consumes snapshots at the start of `RealtimeTrackMixer::process()`. The GUI reports queue rejection without waiting for the callback.

## Existing and added tests

- `tests/realtime_track_mixer_tests.cpp`: validates block-boundary application, pan/gain behavior, mute/solo behavior, invalid input rejection, and bounded queue rejection. It is engine-level, not Qt UI coverage.
- `tests/gui_engine_command_path_tests.cpp`: deterministic synthetic-buffer coverage for the project/OperationLog → track-control queue → next processing block flow, including preview, commit, undo/redo, mute/solo, and queue-full behavior. It mirrors the GUI's track-ID lookup/submission logic; it is not a Qt click test.
- `tests/gui_interaction_tests.cpp` (added on this branch): headless Qt interaction coverage for gain/pan slider commit, mute/solo clicks, undo/redo buttons, and the ALSA button's build-dependent availability. It checks actual widgets and project/history effects. It does **not** claim to observe the private live queue from a GUI click or to test physical audio loopback; those remain open.
- `tests/realtime_bus_engine_tests.cpp`: tests bus fader queue/block behavior, but not the Qt mixer controls.

## GUI/CLI parity

No CLI command surface currently edits a project track's gain, pan, mute, solo, or undo/redo history. `hush_alsa_realtime_host` is a standalone audio host, not a project-editing CLI; the benchmark and soak tools are diagnostic utilities. Therefore GUI/CLI parity for track editing is intentionally **out of scope for this gate**, because there is no existing equivalent CLI action to compare. A project-editing CLI should be designed as a separate feature with shared command semantics rather than inventing undocumented flags here.

Open/Save are local dialog-driven GUI actions and have no CLI parity assertion in this gate. Start/Stop Live ALSA has no equivalent project-aware CLI action; starting the standalone host is not equivalent to the GUI's project-configured live monitor.

## Thread ownership and direct mutation audit

- Qt slots mutate `Project` and `OperationLog` on the GUI/control thread; they do not access or mutate `RealtimeTrackMixer`'s audio-thread-owned gain/pan/flag arrays directly.
- GUI-to-audio state crosses `RealtimeTrackMixer::enqueue_control()` and the bounded SPSC queue only. The callback applies commands at the beginning of `process()`.
- The callback uses preallocated track/output buffers and does not allocate, lock, log, or perform I/O.
- The Qt test verifies widget actions change project/history state and the separate deterministic command-path test verifies the engine sees queued snapshots on a later block. Neither test alone proves the end-to-end widget-to-queue call occurred for a physical live session; that stronger assertion and loopback coverage remain open.

## Gate status

**PARTIAL, not closed.** This branch adds actual headless Qt widget interaction tests and preserves the deterministic queue/engine contract test. A green CI run is required before treating the new test as passing. Queue-observation injection at the exact GUI signal boundary, a dedicated two-channel loopback test, and target-device validation remain outstanding. Do not claim hardware validation, zero xruns, or CLI parity beyond the documented scope above.
