# Release gate ledger

This ledger distinguishes source/CI evidence from tests that require a real target machine. A green compile is not evidence of latency, zero xruns, or long-duration stability.

| Gate | Current evidence | Status |
|---|---|---|
| C++20 headless configure/build/contracts | GitHub Actions [run 39255426377](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39255426377), job `linux-cxx20` | Pass on pre-Gate-7 baseline; new branch revision still needs CI |
| ALSA-enabled configure/build/contracts | Same run, job `linux-alsa-enabled`; compile/contracts only | Pass on pre-Gate-7 baseline; new host changes still need CI; not hardware validation |
| Qt 6 mixer configure/build/contracts | Same run, job `linux-qt6-mixer-gui` | Pass on pre-Gate-7 baseline; GUI lifecycle changes still need CI; Qt interaction test remains open |
| Qt 6 + ALSA live mixer | Same run, job `linux-qt6-alsa-live-mixer` | Pass on pre-Gate-7 baseline; GUI lifecycle changes still need CI; audible hardware/loopback verification remains open |
| ASan instrumented CI | Same run, job `linux-asan` | Pass on tested revision; production graph-swap integration/stress remains open |
| TSan instrumented CI | Same run, job `linux-tsan` | Pass on tested revision; production graph-swap integration/race coverage remains open |
| Repository artifact hygiene | Same run, job `repo-artifact-hygiene` | Pass on tested revision; legacy ZIP disposition still requires content review |
| GUI-to-live-engine command path | Optional Start/Stop Live ALSA action queues GUI model controls and undo/redo through bounded SPSC commands; capture L/R feed tracks 1–2 only | Source implemented; GUI interaction test, CLI parity, and target-device checks open |
| GUI fader command applies at audio block boundary | Existing `RealtimeTrackMixer` applies queued controls at process block boundaries; no Qt interaction/loopback test | Partial; UI-level evidence open |
| Safe graph swap and deferred reclamation | `RealtimeGraphPublisher` primitive supports guarded readers and control-thread reclamation; test now includes a held-reader lifetime assertion and concurrent publication stress | Primitive only; test result for latest commit pending; engine integration remains open |
| Full track/bus/master routing, insert and solo/mute behavior | Offline bus primitives exist; full track-to-live-graph semantics are incomplete | Open |
| GUI meter and xrun polling | GUI polls callback/recovery counters from the ALSA backend; peak/RMS meters are not implemented | Partial; target-device verification open |
| ALSA hardware startup/shutdown and callback timing | No target-device artifact attached | Open |
| 60-minute zero-xrun run | No target-device artifact attached | Open |
| Loopback latency and underrun injection | No measurement report | Open |
| 500-track performance budget | `tools/hush_routing_benchmark.cpp` and CMake target added; no qualifying report attached | Partial; 500-track run required |
| 8-hour soak and memory-drift bound | `tools/hush_headless_soak.cpp` emits JSON counters/RSS where available; no 8-hour report or pass threshold attached | Partial; run and threshold required |
| Process-crash recovery | Completed-record restart and truncated-tail behavior have existing coverage | Partial; power-loss durability not claimed |
| Partial-write fault injection | `tests/recovery_partial_write_tests.cpp` includes interrupted-header and truncated-payload cases; CI passed these in run 39255426377 | Pass on tested revision; power-loss durability not claimed |
| Autosave lifecycle | PR #36 adds dirty-aware policy, typed save errors, an injectable journal writer, GUI open/save/quit/timer hooks, CLI project-open/recovery handling, and lifecycle contract tests | In progress; fresh CI has not yet reported for this branch; Qt close-dialog and CLI dirty-quit failure tests remain open |
| Legacy ZIP disposition | Committed ZIP remains; content has not yet been reviewed in this gate ledger | Open; do not delete or relocate before review |

## Report requirements

Target-machine reports should be JSON and include at least: commit SHA, OS/kernel, CPU, audio interface and driver, sample rate, period size, duration, xrun count, latency method/results, memory measurements, test command, and pass/fail. Missing hardware metadata or an unrun gate must be marked `not_run`, never inferred as passing.

See [PHASE_1_TO_7_EXECUTION.md](PHASE_1_TO_7_EXECUTION.md) for the seven-phase completion ledger.
