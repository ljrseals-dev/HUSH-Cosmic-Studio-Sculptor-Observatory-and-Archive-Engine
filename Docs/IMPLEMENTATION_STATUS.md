# HUSH implementation status

## Implemented and covered by contract tests/CI
- [x] C++20 offline CMake build definition.
- [x] Schema-v1 project model and JSON round-trip for tracks, buses, and routes; older schema-v1 files without bus/route arrays remain readable.
- [x] Bus-routing validation at project-load boundary, including unknown endpoints, duplicate/empty IDs, self-routes, invalid gain, and cycles.
- [x] Append-only recovery journal with length-framed full-project snapshots and truncated-tail recovery.
- [x] Autosave scheduling policy; caller must poll from a non-audio thread.
- [x] Invertible operation-log primitives and undo/redo tests.
- [x] Bus graph validation, send tap metadata, deterministic offline/reference rendering.
- [x] Fixed-capacity SPSC parameter queue and preallocated bus processor.
- [x] RealtimeBusEngine applies queued fader changes at processing-block boundaries.
- [x] Interleaved-stereo audio adapter with preallocated scratch.
- [x] Optional ALSA backend/host compiles and contract tests run in CI; this is not hardware validation.
- [x] Optional Qt 6 mixer shell and combined Qt + ALSA target are implemented in source; the live monitor is explicitly started by the user and GUI edits/undo/redo enqueue bounded realtime track-mixer commands. All seven jobs passed in [GitHub Actions run 39255426377](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39255426377) on the tested revision; later commits require a fresh run. Hardware validation is not implied.
- [x] Header-only RealtimeGraphPublisher primitive and a concurrent publication test exist. The test is being strengthened to assert that an old snapshot is not destroyed while a reader guard holds it; the primitive is not integrated into the production bus/audio engine.
- [x] Repository hygiene files: .gitignore, CHANGELOG.md, ROADMAP.md, and RELEASE_GATES.md.

## Open gates — do not claim complete
- [ ] GUI interaction test proving fader/pan/mute/solo and undo/redo are audible at the next callback block; source queues controls for the explicitly started optional ALSA monitor, but automated UI/loopback evidence and GUI/CLI parity remain open.
- [ ] Safe graph swap integrated into the actual engine and deferred reclamation verified under callback load; the standalone publication primitive is not production integration.
- [ ] Track-to-bus/master routing, insert placement, end-to-end mute/solo behavior, peak/RMS meters, and complete xrun reporting.
- [ ] Re-run ASan/TSan after the latest source changes and add race coverage for production graph-swap integration.
- [ ] Real-device ALSA startup/shutdown, callback timing, lock-free atomics verification on target, and xrun reporting.
- [ ] 60-minute zero-xrun run, loopback latency, and underrun injection on a named target device.
- [ ] 500-track performance report with configurable budget.
- [ ] 8-hour soak report with an explicit memory-growth bound.
- [ ] Power-loss durability: explicitly not claimed. Current coverage is process-restart recovery of completed records and truncated-tail handling.
- [ ] Autosave controller integrated into an application lifecycle with save-failure surfacing.
- [ ] Review the contents and intended disposition of the committed `hush-daw-codex-hush-processor-integration-starter (1).zip`; do not delete or relocate it before review.
- [ ] Optional JACK/PipeWire backends and preservation of historical 0.1.0 tests are not present in this source archive.

A green CI job establishes only the configuration and tests that actually ran. It does not prove hardware latency, zero xruns, soak stability, or release readiness.
