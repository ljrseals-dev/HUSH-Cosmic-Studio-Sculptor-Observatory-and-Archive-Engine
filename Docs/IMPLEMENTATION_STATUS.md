# HUSH implementation status

## Implemented in source; validation pending for this branch
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
- [x] Optional ALSA backend/host source and optional Qt 6 mixer shell source.
- [x] Initial transport core: bounded GUI/control-to-audio command queue for play, stop, pause, seek; audio-thread-owned sample clock advances at block boundaries.
- [x] Initial tempo-map utility: bounded tempo-point storage and sample/beat conversion.
- [x] Initial automation-lane utility: validated ordered points, endpoint hold, step and linear interpolation; intended to be immutable during playback.
- [x] Unit-test source registered for transport, tempo-map, and automation utilities.

**Evidence boundary:** the transport/automation additions above are committed to this branch, but no local CMake build, CTest, sanitizer run, GUI run, or audio-device test has been completed in this environment. The new modules must not be described as build-verified or production-integrated until those checks run.

## Open gates — do not claim complete
- [ ] Run the baseline CMake configure/build/CTest and verify the newly added tests; resolve compiler/test failures before further integration.
- [ ] Connect transport state and sample clock to a running audio callback and GUI transport controls.
- [ ] Connect automation lanes to actual track/bus parameter application during playback, with clear lane ownership/lifetime and no concurrent mutation.
- [ ] Persist tempo maps and automation data with an explicit schema migration and backward-compatible schema-v1 reader; connect edits to OperationLog undo/redo.
- [ ] GUI fader/pan/mute/solo commands queued to the live engine at block boundaries, plus GUI interaction test and GUI/CLI parity.
- [ ] Define and implement meter/time-signature and bar/beat/tick display semantics, loop behavior, and seek policy.
- [ ] Safe graph swap and deferred reclamation; graph remains configure-once.
- [ ] Track-to-bus/master routing, insert placement, end-to-end mute/solo behavior, meters, and xrun counters.
- [ ] ASan/TSan CI jobs pass and provide useful evidence for current code; no graph swap exists to stress yet.
- [ ] Real-device ALSA startup/shutdown, callback timing, lock-free atomics verification on target, and xrun reporting.
- [ ] 60-minute zero-xrun run, loopback latency, and underrun injection on a named target device.
- [ ] 500-track performance report with configurable budget.
- [ ] 8-hour soak report with memory-growth bound.
- [ ] Power-loss durability: explicitly not claimed. Current coverage is process-restart recovery of completed records and truncated-tail handling.
- [ ] Autosave controller integrated into an application lifecycle with save-failure surfacing.
- [ ] Optional JACK/PipeWire backends and preservation of historical 0.1.0 tests not present in this source archive.

A green CI job establishes only the configuration and tests that actually ran. It does not prove hardware latency, zero xruns, soak stability, or release readiness.
