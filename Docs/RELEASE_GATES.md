# Release gate ledger

This ledger distinguishes source/CI evidence from tests that require a real target machine. A green compile is not evidence of latency, zero xruns, or long-duration stability.

| Gate | Current evidence | Status |
|---|---|---|
| C++20 headless configure/build/contracts | GitHub Actions [run 39255426377](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39255426377), job `linux-cxx20` | Pass on tested revision; re-run after subsequent commits |
| ALSA-enabled configure/build/contracts | Same run, job `linux-alsa-enabled`; compile/contracts only | Pass on tested revision; not hardware validation |
| Qt 6 mixer configure/build/contracts | Same run, job `linux-qt6-mixer-gui` | Prior run passed; new headless Qt widget interaction target is now required by CI, fresh result pending |
| Qt 6 + ALSA live mixer | Same run, job `linux-qt6-alsa-live-mixer` | Pass on tested revision; audible hardware/loopback verification remains open |
| ASan instrumented CI | Same run, job `linux-asan` | Pass on tested revision; fresh CI for the newly integrated host path remains open |
| TSan instrumented CI | Same run, job `linux-tsan` | Pass on tested revision; production graph-swap integration/race coverage remains open |
| Repository artifact hygiene | Same run, job `repo-artifact-hygiene` | Pass on tested revision; legacy ZIP disposition still requires content review |
| GUI-to-live-engine command path | Optional Start/Stop Live ALSA action queues GUI model controls and undo/redo through bounded SPSC commands; startup snapshot seeding rejects track-count mismatches before enqueueing; capture L/R feed tracks 1–2 only | Synthetic queue/DSP contract test and new headless Qt widget/history interaction test added; fresh CI pending. Widget test does not run ALSA; observable live-adapter proof, CLI parity, and target-device checks remain open |
| GUI fader command applies at audio block boundary | Existing `RealtimeTrackMixer` applies queued controls at process block boundaries; no Qt interaction/loopback test | Partial; UI-level evidence open |
| Safe graph swap and deferred reclamation | `RealtimeGraphPublisher` primitive supports guarded readers and control-thread reclamation; test includes held-reader lifetime assertion and concurrent publication stress; compile-time assertions now reject non-lock-free pointer/bool atomics | Engine wrapper is integrated into the standalone ALSA host; guarded master-output copy and concurrent replacement tests added; fresh CI pending; GUI topology publication and target hardware evidence remain open |
| Full track/bus/master routing, insert and solo/mute behavior | New `RealtimeTrackBusRouter` core stage maps configured mono tracks to bus inputs with block-boundary gain/pan/mute/solo; deterministic contract test added. Runtime-only mapping; no schema-v1 field added. Not integrated into published engine/Qt/ALSA live path yet. | Partial; fresh CI pending |
| GUI meter and xrun polling | GUI polls callback/recovery counters and displays lock-free stereo block peak/RMS values from the track mixer | Source/test implementation added; fresh CI and target-device verification open; meters are linear sample peak/RMS, not true-peak/loudness |
| ALSA hardware startup/shutdown and callback timing | No target-device artifact attached | Open |
| 60-minute zero-xrun run | No target-device artifact attached | Open |
| Loopback latency and underrun injection | No measurement report | Open |
| 500-track performance budget | `tools/hush_routing_benchmark.cpp` and CMake target added; no qualifying report attached | Partial; 500-track run required |
| 8-hour soak and memory-drift bound | `tools/hush_headless_soak.cpp` emits JSON counters/RSS where available; no 8-hour report or pass threshold attached | Partial; run and threshold required |
| Process-crash recovery | Completed-record restart and truncated-tail behavior have existing coverage | Partial; power-loss durability not claimed |
| Partial-write fault injection | `tests/recovery_partial_write_tests.cpp` includes interrupted-header and truncated-payload cases; CI passed these in run 39255426377 | Pass on tested revision; power-loss durability not claimed |
| Autosave lifecycle | GUI timer invokes the controller on the UI thread; local recovery prompt on startup; manual save/load clears stale journal; close attempts final snapshot and surfaces failures | Implemented in source; fresh Qt CI pending; power-loss durability not claimed |
| Legacy ZIP disposition | Committed ZIP remains; content has not yet been reviewed in this gate ledger | Open; do not delete or relocate before review |

## Report requirements

Target-machine reports should be JSON and include at least: commit SHA, OS/kernel, CPU, audio interface and driver, sample rate, period size, duration, xrun count, latency method/results, memory measurements, test command, and pass/fail. Missing hardware metadata or an unrun gate must be marked `not_run`, never inferred as passing.

See [PHASE_1_TO_7_EXECUTION.md](PHASE_1_TO_7_EXECUTION.md) for the seven-phase completion ledger.
