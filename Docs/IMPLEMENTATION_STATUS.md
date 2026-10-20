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
- [x] GUI committed track-control updates now retain a pending track ID when the bounded realtime queue is full and retry the latest project snapshot from the UI timer without spinning or blocking. Qt widget-click and hardware loopback tests remain open.
- [x] Added `hush_gui_engine_command_path_tests`: deterministic synthetic-buffer contract coverage for preview, commit, undo/redo, mute/solo, block-boundary command delivery, and bounded-queue rejection using the production `OperationLog` and `RealtimeTrackMixer` APIs. This is not a Qt widget-click test or hardware loopback test; the newly added test still needs CI execution.
- [x] Header-only RealtimeGraphPublisher primitive and held-reader/concurrent publication tests.
- [x] Added PublishedRealtimeBusEngine: replacement RealtimeBusEngine instances are configured on the control thread, published for callback processing, guarded for the full process call, and reclaimed later on the control thread. A concurrent engine-replacement contract test is registered. The standalone ALSA host now uses the published engine and a new preallocated adapter; `process_to_master()` copies master output while the graph lifetime guard is held. Fresh CI is pending. The GUI does not yet publish bus-graph topology.
- [x] Repository hygiene files: .gitignore, CHANGELOG.md, ROADMAP.md, and RELEASE_GATES.md.

## Open gates — do not claim complete
- [ ] Qt widget-interaction test proving actual fader/pan/mute/solo and undo/redo signals invoke the command path. The headless command/engine contract test is added, but must pass CI; UI click-level and loopback evidence and GUI/CLI parity remain open.
- [ ] Run fresh ASan/TSan/ALSA CI for the newly integrated published-engine host path; GUI topology publication and target-hardware evidence remain open.
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
