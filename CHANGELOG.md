# Changelog

Notable repository changes. This is a development changelog, not a claim of a finished DAW release.

## 0.2.0 — headless foundation

- C++20 CMake project with schema-v1 persistence, bus graph validation, offline/reference rendering, preallocated bus processing, and a bounded SPSC parameter queue.
- Added realtime stereo adapter and optional ALSA backend/host.
- Added recovery journal, autosave policy controller, and operation-log undo/redo primitives.
- Added optional Qt 6 mixer shell and an explicitly started ALSA live-input monitor when both GUI and ALSA options are enabled; GUI fader/pan/mute/solo edits and undo/redo enqueue bounded realtime track-mixer commands. Current live input mapping is limited to stereo capture channels on the first two tracks.
- Added CI for default C++20, ALSA-enabled, Qt 6 GUI, combined Qt + ALSA live-mixer build, ASan, and TSan configurations.
- Added release-gate ledger and documented unverified hardware/performance/durability gates.

## Unreleased — offline project bus-to-WAV export

- Connected schema-v1 project bus/route validation and the offline bus renderer to PCM16/float32 WAV output through an explicit selected output bus.
- Added contract coverage for routing-to-WAV samples and invalid output-bus/project cases. CI validation is pending; this does not yet load media files or render an arrangement timeline.

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

- Made bounded-queue rejection transactional for GUI edits, undo, and redo; failed enqueue no longer advances project state/history.
- Added `RealtimeBusEngineHost` to publish fully configured bus-engine snapshots off the audio callback and defer retired graph destruction until the reader guard is clear; added concurrent processing/publication coverage. CI and sanitizer results for this new host are pending.

- Made live GUI edits, undo, and redo transactional with respect to bounded-queue acceptance: if the queue is full, the operation is not committed to project state/history and the UI restores canonical values.
- Expanded realtime track-mixer tests for non-finite values, invalid track indices, upper/lower bounds, block-boundary output changes, and queue saturation. Qt interaction and physical loopback tests remain open.

- Connected GUI model edits and undo/redo to the preconfigured realtime track mixer through its bounded SPSC queue.
- Added explicit Start/Stop Live ALSA controls; callback buffers are prepared before start and backend shutdown joins the callback before releasing mixer state.
- Documented the current limitation: stereo capture channels feed only the first two tracks; full track-source routing, GUI interaction tests, loopback, and hardware/soak validation remain outstanding.
