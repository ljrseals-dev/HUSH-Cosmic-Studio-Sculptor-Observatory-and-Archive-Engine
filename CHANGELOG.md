# Changelog

Notable repository changes. This is a development changelog, not a claim of a finished DAW release.

## 0.2.0 — headless foundation

- C++20 CMake project with schema-v1 persistence, bus graph validation, offline/reference rendering, preallocated bus processing, and a bounded SPSC parameter queue.
- Added realtime stereo adapter and optional ALSA backend/host.
- Added recovery journal, autosave policy controller, and operation-log undo/redo primitives.
- Added optional Qt 6 mixer shell and an explicitly started ALSA live-input monitor when both GUI and ALSA options are enabled; GUI fader/pan/mute/solo edits and undo/redo enqueue bounded realtime track-mixer commands. Current live input mapping is limited to stereo capture channels on the first two tracks.
- Added CI for default C++20, ALSA-enabled, Qt 6 GUI, combined Qt + ALSA live-mixer build, ASan, and TSan configurations.
- Added release-gate ledger and documented unverified hardware/performance/durability gates.

## Unreleased — Phase 1–7 execution tooling

- Added configurable synthetic routing benchmark and headless mixer/recovery-journal soak executables with JSON output.
- Added recovery journal tests for interrupted record headers and truncated payloads. These additions have not yet been run in this session; performance and soak reports remain outstanding.

## Unreleased — repository hygiene

- Added tracked-file, historical rename, and archive audit in `Docs/REPO_AUDIT.md`.
- Removed four malformed 2-byte ZIP placeholders and the unreferenced `images/1792375771560.jpg`.
- Preserved two non-empty archives whose full contents could not be verified; they remain explicit audit debt and must not be treated as source dependencies.
- Added artifact ignore rules, contribution guidance, and security reporting policy.
- Published history remains intact; no filter-repo, force-push, or history rewrite was performed.

## Unreleased — GUI live mixer wiring

- Connected GUI model edits and undo/redo to the preconfigured realtime track mixer through its bounded SPSC queue.
- Added explicit Start/Stop Live ALSA controls; callback buffers are prepared before start and backend shutdown joins the callback before releasing mixer state.
- Documented the current limitation: stereo capture channels feed only the first two tracks; full track-source routing, GUI interaction tests, loopback, and hardware/soak validation remain outstanding.

## Unreleased — headless Qt interaction coverage

- Added `tests/gui_interaction_tests.cpp` to exercise actual gain/pan slider commits, mute/solo clicks, undo/redo buttons, and live-button availability using Qt's offscreen platform.
- Added stable widget object names and test-only project/history accessors compiled only for the GUI test target.
- Added `Docs/GUI_INTERACTION_TEST_AUDIT.md` documenting queue payload fields, current coverage, CLI-parity scope, and the remaining loopback/hardware gates. CI execution for the new test is pending; no hardware validation is claimed.
