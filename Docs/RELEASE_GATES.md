# Release gate ledger

This ledger distinguishes source/CI evidence from tests that require a real target machine. A green compile is not evidence of latency, zero xruns, or long-duration stability.

| Gate | Current evidence | Status |
|---|---|---|
| C++20 headless configure/build/contracts | GitHub Actions [run 39273263409](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39273263409), job `linux-cxx20` | Pass on tested revision; re-run after subsequent commits |
| ALSA-enabled configure/build/contracts | Same run, job `linux-alsa-enabled`; compile/contracts only | Pass on tested revision; not hardware validation |
| Qt 6 mixer configure/build/contracts | Same run, job `linux-qt6-mixer-gui`; new `hush_qt_mixer_interaction_tests` added on `feat/hush-0.3a-qt-widget-tests` | Historical build passed; fresh CI for actual widget interactions pending |
| Qt 6 + ALSA live mixer | Same run, job `linux-qt6-alsa-live-mixer` | Pass on tested revision; audible hardware/loopback verification remains open |
| ASan instrumented CI | Same run, job `linux-asan` | Pass on tested revision; production graph-swap integration/stress remains open |
| TSan instrumented CI | Same run, job `linux-tsan` | Pass on tested revision; production graph-swap integration/race coverage remains open |
| Repository artifact hygiene | Same run, job `repo-artifact-hygiene` | Pass on tested revision; legacy ZIP disposition still requires content review |
| GUI-to-live-engine command path | Explicit Start/Stop Live ALSA; edits/undo/redo enqueue bounded SPSC snapshots before project/history mutation; core mixer tests cover block-boundary application and rejection. New offscreen Qt widget test covers actual slider-release, mute/solo, Undo/Redo signals against project/history | Widget test added; fresh CI passed in [run 39469302105](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39469302105); actual live queue integration, loopback, CLI parity, and target-device checks remain open |
| GUI fader command applies at audio block boundary | `RealtimeTrackMixer` applies queued controls at process block boundaries; regression tests assert output changes and deterministic queue saturation rejection; no Qt interaction/loopback test | Core engine contract covered in run 39273263409; GUI-level evidence remains open |
| Safe graph swap and deferred reclamation | `RealtimeBusEngineHost` publishes configured snapshots off-callback; the ALSA host now routes its callback through a preallocated host adapter. A concurrent adapter/graph-replacement test is registered in CMake | Earlier engine-host tests passed in [run 39274233768](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39274233768); this branch's adapter test awaits fresh CI, and target-device validation remains open |
| Full track/bus/master routing, insert and solo/mute behavior | Optional `Track::bus_id` persists and validates; `RealtimeProjectAudioPipeline` routes configured source tracks to master and exposes atomic pre-graph bus-input meters. Bus gain/mute/solo now persist in schema-v1 and have bounded block-boundary engine commands; solo keeps downstream route paths audible. GUI bus widgets, insert placement, post-fader/insert meters, extra media/device sources, and hardware evidence remain open | Partial; bus-control tests passed all seven jobs in [run 39637315269](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39637315269); target-device/live-loopback tests remain required |
| GUI meter and xrun polling | The Qt live GUI polls input/output peaks, callback/xrun counters, pre-graph bus-input peak/RMS, and newly added post-fader per-bus peak/RMS snapshots. Insert-output meters and target-device verification remain open | Partial; fresh CI for the new post-fader meter slice required |
| ALSA terminal worker failure reporting | Backend exposes lock-free capture/playback terminal-failure category and raw recovery status; host emits `HUSH_ALSA_WORKER_FAILED` and exits 3 after unrecoverable stream errors | Source change committed on `feat/alsa-terminal-failure-status`; fresh CI and injected/device failure verification required |
| ALSA hardware startup/shutdown and callback timing | No target-device artifact attached | Open |
| 60-minute zero-xrun run | No target-device artifact attached | Open |
| Loopback latency and underrun injection | No measurement report | Open |
| 500-track performance budget | `tools/hush_routing_benchmark.cpp` now samples mixer process timing, reports p95/p99/max and fails when p99 exceeds the configurable budget (default 2666.667 µs); a 500-track CTest smoke test is registered | Harness gate implemented; fresh CI and qualifying full-duration report still required |
| 8-hour soak and memory-drift bound | `tools/hush_headless_soak.cpp` now enforces a configurable positive RSS-drift threshold (default 64 MiB); CTest smoke test checks JSON status and journal write | Threshold enforcement implemented; fresh CI and 28,800-second report still required |
| Process-crash recovery | Completed-record restart and truncated-tail behavior have existing coverage | Partial; power-loss durability not claimed |
| Partial-write fault injection | `tests/recovery_partial_write_tests.cpp` includes interrupted-header and truncated-payload cases; CI passed these in run 39255426377 | Pass on tested revision; power-loss durability not claimed |
| Autosave lifecycle | ALSA CLI host and Qt GUI poll the controller off the callback; GUI prompts before recovery and honors explicit discard; failures are surfaced and retried after a delay. Offscreen Qt tests now cover accept/decline recovery, declined journal removal, and autosave polling; all seven jobs passed in [run 39636175080](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-and-Archive-Engine/actions/runs/39636175080) | Partial; host failure-path tests and hardware recovery qualification required |
| Legacy ZIP disposition | Committed ZIP remains; content has not yet been reviewed in this gate ledger | Open; do not delete or relocate before review |

## Report requirements

Target-machine reports should be JSON and include at least: commit SHA, OS/kernel, CPU, audio interface and driver, sample rate, period size, duration, xrun count, latency method/results, memory measurements, test command, and pass/fail. Missing hardware metadata or an unrun gate must be marked `not_run`, never inferred as passing.

See [PHASE_1_TO_7_EXECUTION.md](PHASE_1_TO_7_EXECUTION.md) for the seven-phase completion ledger.


## HUSH 0.4a recording gates

| Gate | Evidence | Status |
|---|---|---|
| Preallocated capture ring | PR #75 adds SPSC interleaved-float ring and boundary/wrap/overflow tests; CI run [39408852666](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39408852666) | Pending while CI runs |
| Off-thread streaming WAV writer and finalized file header | Explicit ALSA CLI `--record FILE.wav` uses the SPSC capture ring and background `RecordingSession`/`WavStreamWriter`; pipeline test checks final WAV header and frame count | Source/test CI passed in [run 39549079073](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39549079073) and [run 39549079415](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39549079415); physical-device/dropped-frame qualification required |
| Track arming, take IDs/playlists, punch, comping, crossfades | Not implemented in this slice | Open |
| 8 tracks × 10 minutes, sample-accurate lengths, no dropped frames | No qualifying run | Open |
| Punch accuracy ±1 sample, latency compensation/loopback | No device measurement | Open |


## Phase 5 harness contract

See [PERFORMANCE_SOAK_GATE.md](PERFORMANCE_SOAK_GATE.md) for commands, JSON fields, thresholds, exit codes, and the distinction between smoke coverage and qualifying long-duration evidence. Both tools explicitly report `hardware_audio_test: false`.
