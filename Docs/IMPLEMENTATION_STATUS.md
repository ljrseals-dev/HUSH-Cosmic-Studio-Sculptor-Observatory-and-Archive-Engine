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
- [x] Added `RealtimeTrackBusRouter`: preallocated mono-track-to-bus summing with bounded block-boundary gain/pan/mute/solo commands and deterministic tests. Runtime mapping is explicit because schema v1 does not persist a per-track output bus; live Qt/ALSA integration remains open.
- [x] Interleaved-stereo audio adapter with preallocated scratch.
- [x] Optional ALSA backend/host compiles and contract tests run in CI; this is not hardware validation.
- [x] Optional Qt 6 mixer shell and combined Qt + ALSA target are implemented in source; the live monitor is explicitly started by the user and GUI edits/undo/redo enqueue bounded realtime track-mixer commands. All seven jobs passed in [GitHub Actions run 39255426377](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39255426377) on the tested revision; later commits require a fresh run. Hardware validation is not implied.
- [x] GUI committed track-control updates now retain a pending track ID when the bounded realtime queue is full and retry the latest project snapshot from the UI timer without spinning or blocking.
- [x] Added headless Qt widget interaction coverage for gain/pan slider-release commits, mute/solo clicks, and undo/redo state restoration. CI explicitly requires this target in both Qt-only and Qt+ALSA builds. This verifies widget-to-project/history wiring; it does not simulate a running ALSA device or prove audible hardware output.
- [x] Realtime track mixer publishes stereo block peak/RMS diagnostics through lock-free 32-bit atomics; deterministic synthetic-buffer tests verify peak and RMS values. The live GUI status timer displays these alongside callback and xrun/recovery counters. These are linear sample peak/RMS diagnostics, not true-peak meters or hardware stability evidence.\n- [x] Added `hush_gui_engine_command_path_tests`: deterministic synthetic-buffer contract coverage for preview, commit, undo/redo, mute/solo, block-boundary command delivery, and bounded-queue rejection using the production `OperationLog` and `RealtimeTrackMixer` APIs. Startup snapshot seeding now rejects project/engine track-count mismatches and snapshots larger than queue capacity before enqueueing any commands; a regression test verifies mismatches do not enqueue partial state. This is not a Qt widget-click test or hardware loopback test; the follow-up commit still needs fresh CI.
- [x] Header-only RealtimeGraphPublisher primitive and held-reader/concurrent publication tests.
- [x] Added PublishedRealtimeBusEngine: replacement RealtimeBusEngine instances are configured on the control thread, published for callback processing, guarded for the full process call, and reclaimed later on the control thread. The standalone ALSA host uses the published engine and preallocated adapter; `process_to_master()` copies master output while the graph lifetime guard is held. The concurrent replacement test now drives `process_to_master()` while 2,000 graph replacements occur and checks every copied sample against fixed input data, with failure counters asserted after join. This extends automated coverage of the actual device-output copy path; fresh CI for this test revision is pending. The GUI does not yet publish bus-graph topology.
- [x] Repository hygiene files: .gitignore, CHANGELOG.md, ROADMAP.md, and RELEASE_GATES.md.

## Open gates — do not claim complete
- [ ] Qt widget-interaction evidence for the complete widget-to-live-engine path. New headless Qt tests cover gain/pan slider-release, mute/solo clicks, and undo/redo model/history behavior; the separate synthetic command-path test covers the bounded queue and next-block DSP result. The missing link is a Qt test with an injectable/observable live-engine adapter, plus hardware loopback evidence and GUI/CLI parity.
- [ ] Run fresh ASan/TSan/ALSA CI for the newly integrated published-engine host path; GUI topology publication and target-hardware evidence remain open.
- [ ] Track-to-bus/master routing, insert placement, end-to-end mute/solo behavior across the bus graph, and complete xrun reporting. Track-mixer stereo sample peak/RMS diagnostics are implemented; true-peak/loudness and target-device validation remain out of scope.
- [ ] Re-run ASan/TSan after the latest source changes and add race coverage for production graph-swap integration.
- [ ] Real-device ALSA startup/shutdown, callback timing, lock-free atomics verification on target, and xrun reporting.
- [ ] 60-minute zero-xrun run, loopback latency, and underrun injection on a named target device.
- [ ] 500-track performance report with configurable budget.
- [ ] 8-hour soak report with an explicit memory-growth bound.
- [ ] Power-loss durability: explicitly not claimed. Current coverage is process-restart recovery of completed records and truncated-tail handling.
- [x] Qt mixer lifecycle now polls the existing autosave controller on the GUI thread, writes snapshots only to the local application-data recovery journal, offers an explicit startup recovery choice, flushes and clears the recovery journal after a successful manual project save/load, and attempts a final recovery snapshot on close. Autosave write failures are surfaced in the status bar; close-time failure asks before exiting. This is source integration, not power-loss durability, and fresh CI for this commit is pending.
- [ ] Review the contents and intended disposition of the committed `hush-daw-codex-hush-processor-integration-starter (1).zip`; do not delete or relocate it before review.
- [ ] Optional JACK/PipeWire backends and preservation of historical 0.1.0 tests are not present in this source archive.

A green CI job establishes only the configuration and tests that actually ran. It does not prove hardware latency, zero xruns, soak stability, or release readiness.
