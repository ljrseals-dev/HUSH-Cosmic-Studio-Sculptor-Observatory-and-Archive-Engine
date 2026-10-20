# HUSH implementation status

## Implemented and covered by contract tests/CI
- [x] C++20 offline CMake build definition.
- [x] Schema-v1 project model and JSON round-trip for tracks, buses, and routes; older schema-v1 files without bus/route arrays remain readable.
- [x] Bus-routing validation at project-load boundary, including unknown endpoints, duplicate/empty IDs, self-routes, invalid gain, and cycles.
- [x] Append-only recovery journal with length-framed full-project snapshots and truncated-tail recovery.
- [x] Autosave scheduling policy and Qt mixer lifecycle integration: the GUI polls on a non-audio timer, writes project-adjacent local recovery snapshots for named projects, offers recovery on open, and surfaces journal failures. The offscreen Qt test now covers an edit-to-journal round trip; fresh CI is pending. This recovery journal does not replace saving the project file, and power-loss durability is not claimed.
- [x] Invertible operation-log primitives and undo/redo tests.
- [x] Bus graph validation, send tap metadata, deterministic offline/reference rendering.
- [x] Fixed-capacity SPSC parameter queue and preallocated bus processor.
- [x] RealtimeBusEngine applies queued fader changes at processing-block boundaries.
- [x] Interleaved-stereo audio adapter with preallocated scratch.
- [x] Optional ALSA backend/host compiles and contract tests run in CI; this is not hardware validation.
- [x] Optional Qt 6 mixer shell and combined Qt + ALSA target are implemented in source; the live monitor is explicitly started by the user and GUI edits/undo/redo enqueue bounded realtime track-mixer commands. Queue rejection now happens before project/history mutation, and the UI restores canonical values after rejection. Core tests cover block-boundary application, saturation, invalid indices, non-finite values, and range rejection. All seven jobs passed on the code/test revision in [CI run 39273263409](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39273263409), including Qt + ALSA, ASan, TSan, and artifact hygiene. A later commit updates only evidence documentation. Hardware validation is not implied.
- [x] `hush_gui_engine_command_path_tests` exercises preview, commit, undo/redo, pan, mute/solo, stable-ID lookup, and queue-full transactional rejection through production project/history/mixer APIs. It is headless core coverage, not actual Qt widget-click or physical loopback evidence; CI for this new target is pending.
- [x] `RealtimeGraphPublisher` now fails closed on overlapping reader guards and defers retired snapshot destruction to the control thread. Added `RealtimeBusEngineHost`, which builds/validates a replacement `RealtimeBusEngine` off the callback, publishes the immutable runtime snapshot, holds a reader guard through processing/output copy, and preserves the active graph on invalid replacement. Added an engine-integrated concurrent graph-swap test; all seven jobs passed in [CI run 39274233768](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39274233768), including ASan and TSan.
- [x] Repository hygiene files: .gitignore, CHANGELOG.md, ROADMAP.md, and RELEASE_GATES.md.

## Open gates — do not claim complete
- [x] Added `hush_qt_mixer_interaction_tests`, which runs the actual Qt mixer window offscreen and drives gain/pan slider-release signals, mute/solo clicks, and Undo/Redo; it asserts canonical project state and history cursor changes. Fresh Qt CI is pending. This proves widget-to-model/history behavior, not actual ALSA command delivery or loopback; GUI/CLI parity remains open.
- [ ] The ALSA host now uses `RealtimeBusEngineHost` through `RealtimeBusEngineHostAudioAdapter`, with a concurrent same-topology graph-replacement adapter test registered in CMake. Fresh CI for this branch is pending; real-device lifecycle validation remains separate.
- [ ] Track-to-bus/master routing, insert placement, end-to-end mute/solo behavior, peak/RMS meters, and complete xrun reporting.
- [ ] ASan/TSan passed for the latest source/test revision in [run 39273263409](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39273263409); production graph-swap integration and its race coverage remain open.
- [ ] Real-device ALSA startup/shutdown, callback timing, lock-free atomics verification on target, and xrun reporting.
- [ ] 60-minute zero-xrun run, loopback latency, and underrun injection on a named target device.
- [x] Performance harness now enforces a configurable p99 mixer-block processing budget and emits sampled p95/p99/max JSON; CTest smoke coverage added, fresh CI pending. The qualifying 500-track report is still open.
- [x] Soak harness now enforces a configurable positive RSS-drift bound and emits threshold/result JSON; CTest smoke coverage added, fresh CI pending. The 8-hour report is still open.
- [ ] Power-loss durability: explicitly not claimed. Current coverage is process-restart recovery of completed records and truncated-tail handling.
- [x] Autosave controller integrated into the Qt mixer lifecycle with explicit recovery choice and persistent failure status; offscreen integration test added. Fresh CI remains pending.
- [ ] Review the contents and intended disposition of the committed `hush-daw-codex-hush-processor-integration-starter (1).zip`; do not delete or relocate it before review.
- [ ] Optional JACK/PipeWire backends and preservation of historical 0.1.0 tests are not present in this source archive.

A green CI job establishes only the configuration and tests that actually ran. It does not prove hardware latency, zero xruns, soak stability, or release readiness.


## Recording milestone 0.4a
- [x] Added preallocated SPSC interleaved-float capture ring; capture and drain operate on complete frames without allocation or I/O.
- [x] Overflow is counted as dropped frames and exposed to the control side.
- [ ] Off-thread disk writer, streaming WAV finalization, track arm/disarm, takes/playlists, punch, comping, crossfades, latency compensation, and ALSA capture-to-file integration remain incomplete. Do not claim recording is production-ready.
