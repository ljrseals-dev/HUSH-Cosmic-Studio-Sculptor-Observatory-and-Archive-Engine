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
- [x] Optional Qt 6 mixer shell compiles and contract tests run in CI; the GUI is not yet connected to the live engine.
- [x] Repository hygiene files: .gitignore, CHANGELOG.md, ROADMAP.md, and RELEASE_GATES.md.

## Open gates — do not claim complete
- [ ] GUI fader/pan/mute/solo commands queued to the live engine at block boundaries, plus GUI interaction test and GUI/CLI parity.
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
