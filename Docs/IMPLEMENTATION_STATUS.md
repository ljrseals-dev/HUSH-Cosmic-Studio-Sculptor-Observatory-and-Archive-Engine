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
- [x] Optional ALSA backend/host compiles in CI and reports unrecoverable capture/playback worker failures with the failing side and raw ALSA status; it exits with code 3 after joining the worker. The current CLI/Qt live path now uses `RealtimeProjectAudioPipeline` for project track/bus processing (see the open-gate implementation entry below). Hardware startup/shutdown and target-device behavior remain unverified.
- [x] Optional Qt 6 mixer shell and combined Qt + ALSA target are implemented in source; live monitoring is explicitly user-started. GUI edits/undo/redo enqueue bounded control snapshots into the project-aware track-to-bus pipeline before project/history mutation, and queue rejection restores canonical values. The pipeline's source/test changes passed the seven-job CI run [39548627190](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39548627190). Automated Qt recovery/autosave interaction coverage and physical loopback/hardware validation remain open.
- [x] `hush_gui_engine_command_path_tests` exercises preview, commit, undo/redo, pan, mute/solo, stable-ID lookup, and queue-full transactional rejection through production project/history/mixer APIs. It is headless core coverage, not actual Qt widget-click or physical loopback evidence; CI for this new target is pending.
- [x] `RealtimeGraphPublisher` now fails closed on overlapping reader guards and defers retired snapshot destruction to the control thread. Added `RealtimeBusEngineHost`, which builds/validates a replacement `RealtimeBusEngine` off the callback, publishes the immutable runtime snapshot, holds a reader guard through processing/output copy, and preserves the active graph on invalid replacement. Added an engine-integrated concurrent graph-swap test; all seven jobs passed in [CI run 39274233768](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39274233768), including ASan and TSan.
- [x] Repository hygiene files: .gitignore, CHANGELOG.md, ROADMAP.md, and RELEASE_GATES.md.

## Open gates — do not claim complete
- [x] Added `hush_qt_mixer_interaction_tests`, which runs the actual Qt mixer window offscreen and drives gain/pan slider-release signals, mute/solo clicks, and Undo/Redo; it asserts canonical project state and history cursor changes. Fresh Qt CI is pending. This proves widget-to-model/history behavior, not actual ALSA command delivery or loopback; GUI/CLI parity remains open.
- [x] The lower-level `RealtimeBusEngineHostAudioAdapter` remains available and has concurrent same-topology graph-replacement coverage; the ALSA CLI host now uses `RealtimeProjectAudioPipeline` so track buffers, track controls, bus inputs, and master output share one preallocated path. CI passed for the source/test revision in run 39548627190; real-device lifecycle validation remains separate.
- [ ] Full project-integrated track-to-bus/master routing, insert placement, cross-bus mute/solo behavior, and graph-wide peak/RMS meters remain open.
- [x] Added backward-compatible optional schema-v1 `Track::bus_id` persistence/validation and `RealtimeProjectAudioPipeline`, which preallocates mono source buffers, applies bounded block-boundary gain/pan/mute/solo controls, routes tracks into assigned bus inputs, and sends the validated bus graph to the master output. The ALSA CLI host and Qt live mixer now use this pipeline. Empty `bus_id` falls back to the selected input bus; capture L/R feed source tracks 1–2 and additional tracks remain silent until a source is connected. Tests cover JSON round-trip/invalid bus IDs, project graph selection, routed output, and queued mute. The seven-job CI run [39548627190](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39548627190) passed on code/test commit `67c2087c6597071ef4392c097767f5d2cbaf7278`; later changes on this branch are documentation and a comment clarification. Re-run CI on the final head before merge; no physical-device behavior is implied.
- [ ] ASan/TSan passed for the latest source/test revision in [run 39273263409](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39273263409); production graph-swap integration and its race coverage remain open.
- [ ] Real-device ALSA startup/shutdown, callback timing, lock-free atomics verification on target, and xrun reporting.
- [ ] 60-minute zero-xrun run, loopback latency, and underrun injection on a named target device.
- [x] Performance harness now enforces a configurable p99 mixer-block processing budget and emits sampled p95/p99/max JSON; CTest smoke coverage added, fresh CI pending. The qualifying 500-track report is still open.
- [x] Soak harness now enforces a configurable positive RSS-drift bound and emits threshold/result JSON; CTest smoke coverage added, fresh CI pending. The 8-hour report is still open.
- [ ] Power-loss durability: explicitly not claimed. Current coverage is process-restart recovery of completed records and truncated-tail handling.
- [x] ALSA CLI host polls `AutosaveController` on its non-audio control loop, writes recovery snapshots on the configured interval/stop transition, and surfaces journal failures with exit code 2. Qt GUI now polls dirty-project recovery snapshots on a GUI timer, prompts before restoring a newer journal, retries journal failures with a bounded delay, and clears the journal on an explicit discard. Automated Qt recovery/autosave interaction tests and host-level failure-path tests remain open.
- [ ] Review the contents and intended disposition of the committed `hush-daw-codex-hush-processor-integration-starter (1).zip`; do not delete or relocate it before review.
- [ ] Optional JACK/PipeWire backends and preservation of historical 0.1.0 tests are not present in this source archive.

A green CI job establishes only the configuration and tests that actually ran. It does not prove hardware latency, zero xruns, soak stability, or release readiness.


## Recording milestone 0.4a
- [x] Added preallocated SPSC interleaved-float capture ring; capture and drain operate on complete frames without allocation or I/O.
- [x] Overflow is counted as dropped frames and exposed to the control side.
- [ ] Off-thread disk writer, streaming WAV finalization, track arm/disarm, takes/playlists, punch, comping, crossfades, latency compensation, and ALSA capture-to-file integration remain incomplete. Do not claim recording is production-ready.
