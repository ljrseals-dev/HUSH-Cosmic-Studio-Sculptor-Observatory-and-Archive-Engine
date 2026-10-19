# HUSH implementation status

## Implemented and covered by contract tests/CI
- [x] C++20 offline CMake build definition.
- [x] Schema-v1 project model and JSON round-trip for tracks, buses, and routes; older schema-v1 files without bus/route arrays remain readable.
- [x] Bus-routing validation at project-load boundary, including unknown endpoints, duplicate/empty IDs, self-routes, invalid gain, and cycles.
- [x] Append-only recovery journal with length-framed full-project snapshots and truncated-tail recovery.
- [x] Dirty-aware autosave scheduling policy, typed `SaveError` state, synchronous journal save path, and injectable journal-writer seam; all save calls remain non-audio-thread-only. The lifecycle test and all seven CI jobs passed in [run 39291973496](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39291973496).
- [x] Invertible operation-log primitives, undo/redo tests, and dirty tracking on append/apply/undo/redo with explicit `mark_dirty()` / `mark_saved()`.
- [x] Bus graph validation, send tap metadata, deterministic offline/reference rendering.
- [x] Fixed-capacity SPSC parameter queue and preallocated bus processor.
- [x] RealtimeBusEngine applies queued fader changes at processing-block boundaries.
- [x] Interleaved-stereo audio adapter with preallocated scratch.
- [x] Optional ALSA backend/host compiles and contract tests run in CI; this is not hardware validation.
- [x] Optional Qt 6 mixer shell and combined Qt + ALSA target are implemented in source; the live monitor is explicitly started by the user and GUI edits/undo/redo enqueue bounded realtime track-mixer commands. Queue rejection now happens before project/history mutation, and the UI restores canonical values after rejection. Core tests cover block-boundary application, saturation, invalid indices, non-finite values, and range rejection. All seven jobs passed on the code/test revision in [CI run 39273263409](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39273263409), including Qt + ALSA, ASan, TSan, and artifact hygiene. A later commit updates only evidence documentation. Hardware validation is not implied.
- [x] `RealtimeGraphPublisher` now fails closed on overlapping reader guards and defers retired snapshot destruction to the control thread. Added `RealtimeBusEngineHost`, which builds/validates a replacement `RealtimeBusEngine` off the callback, publishes the immutable runtime snapshot, holds a reader guard through processing/output copy, and preserves the active graph on invalid replacement. Added an engine-integrated concurrent graph-swap test; all seven jobs passed in [CI run 39274233768](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39274233768), including ASan and TSan.
- [x] Repository hygiene files: .gitignore, CHANGELOG.md, ROADMAP.md, and RELEASE_GATES.md.

## Open gates — do not claim complete
- [ ] Automated Qt interaction test proving fader/pan/mute/solo and undo/redo are audible at the next callback block; core mixer regression tests cover application, queue saturation, and invalid inputs, but actual Qt interaction/loopback evidence and GUI/CLI parity remain open.
- [ ] The ALSA host now uses `RealtimeBusEngineHost` through `RealtimeBusEngineHostAudioAdapter`, with a concurrent same-topology graph-replacement adapter test registered in CMake. Fresh CI for this branch is pending; real-device lifecycle validation remains separate.
- [ ] Track-to-bus/master routing, insert placement, end-to-end mute/solo behavior, peak/RMS meters, and complete xrun reporting.
- [ ] ASan/TSan passed for the latest source/test revision in [run 39273263409](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39273263409); production graph-swap integration and its race coverage remain open.
- [ ] Real-device ALSA startup/shutdown, callback timing, lock-free atomics verification on target, and xrun reporting.
- [ ] 60-minute zero-xrun run, loopback latency, and underrun injection on a named target device.
- [ ] 500-track performance report with configurable budget.
- [ ] 8-hour soak report with an explicit memory-growth bound.
- [ ] Power-loss durability: explicitly not claimed. Current coverage is process-restart recovery of completed records and truncated-tail handling.
- [ ] Gate 7 lifecycle integration: GUI project open/save/quit and main-thread timer hooks, recovery snapshot handling, and a dirty/error indicator are implemented on the review branch. The CLI accepts `--project` / `--journal`, handles recovery, and documents exit codes, but has no project-edit command surface so dirty-edit/quit-failure behavior is not currently reachable through ordinary use. All seven CI jobs passed in [run 39291973496](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39291973496); Qt close-dialog automation and actual GUI/CLI recovery-notice assertions remain open.
- [ ] Review the contents and intended disposition of the committed `hush-daw-codex-hush-processor-integration-starter (1).zip`; do not delete or relocate it before review.
- [ ] Optional JACK/PipeWire backends and preservation of historical 0.1.0 tests are not present in this source archive.

A green CI job establishes only the configuration and tests that actually ran. It does not prove hardware latency, zero xruns, soak stability, or release readiness.
