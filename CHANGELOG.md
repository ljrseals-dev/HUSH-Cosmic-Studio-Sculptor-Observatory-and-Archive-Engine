## Unreleased — host final-save lifecycle contracts

- Added a reusable non-audio shutdown helper that writes the canonical project file before appending the recovery snapshot, and reports partial failure when the journal write fails.
- Converts project-writer exceptions into typed final-save errors so an injected or unexpected writer exception does not escape the shutdown helper.
- Added contract tests for clean shutdown, missing/throwing writers, project-file failure, journal failure after project-file success, and complete success.
- CI validation is pending; no local build/CTest result is claimed.

## Unreleased — transport command queue boundary tests

- Added regression coverage for FIFO ordering across seek/stop/seek/play commands.
- Added queue-capacity coverage proving a full fixed-capacity queue rejects new commands without overwriting queued work, and that draining restores capacity.
- These checks exercise the existing transport core only; GUI/live-audio transport wiring, automation application, and persistence remain separate integration work.
- Build and CTest have not been run in this GitHub editing workflow; CI validation is required.

## Unreleased — full-range tempo conversion fix

- Replaced the `TempoMap::beat_to_sample` `llround()` conversion with extended-precision accumulation and checked rounding, allowing valid sample positions above `INT64_MAX` while rejecting values at or beyond the exclusive `2^64` bound without modifying the output argument.
- Kept sample-to-beat accumulation in extended precision to reduce avoidable precision loss for large sample positions.
- Added regression checks for valid positions above `INT64_MAX`, out-of-range rejection, and output-argument preservation.
- Source changes are committed; CI/build/CTest validation is still required. No local test pass is claimed.

# Changelog

## Unreleased — transport/automation conflict regression coverage

- Kept the current `main` transport and beat-based automation APIs as the canonical implementation; avoided reintroducing overlapping alternate headers from closed PRs.
- Expanded `hush_transport_automation_tests` to cover sample-clock saturation at `UINT64_MAX`, transactional rejection of invalid tempo-map edits, and preservation of an accepted automation lane after invalid replacement.
- These are source-level regression additions only. Build and CTest evidence must come from CI; no local test pass is claimed.

## Unreleased — voice processor audit baseline

- Documented the current realtime track mixer as gain/pan/mute/solo processing, not a broadcast voice chain.
- Added an offline benchmark harness for ten million frames in 512-frame blocks; measured results, vectorization reports, SIMD comparisons, and DSP-stage validation remain pending.
- No AVX2, broadcast-quality, 96 kHz input/resampling, or hardware-validation claim is made.

## Unreleased

- Add schema-v1-compatible media-source references and sample-positioned audio clips, with track/source reference checks, source-frame bounds validation, and persistence tests. Full arrangement rendering remains a separate step.
- Add offline RIFF/WAVE media import for PCM16 and IEEE float32 files, with bounded memory use, format validation, and decoder contract tests.
- Added MIDI CC-to-parameter mapping foundation with validation and transactional rejection tests. MIDI device I/O, learn-mode GUI, automation playback, and schema-v2 persistence remain out of scope.

Notable repository changes. This is a development changelog, not a claim of a finished DAW release.

## 0.2.0 — headless foundation

- C++20 offline CMake project with schema-v1 persistence, bus graph validation, offline/reference rendering, preallocated bus processing, and a bounded SPSC parameter queue.
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

## Unreleased — MIDI CC mapping foundation

- Added a fixed-capacity MIDI CC-to-parameter mapper with channel/controller validation, linear range scaling, optional inversion, and duplicate-source rejection.
- Added contract tests for mapping endpoints, invalid MIDI events/configuration, and preservation of the previous map after rejected updates.
- MIDI device I/O, learn-mode GUI, parameter command delivery, automation playback, and schema-v2 persistence remain outside this increment. Tests have not yet been run in this environment.
