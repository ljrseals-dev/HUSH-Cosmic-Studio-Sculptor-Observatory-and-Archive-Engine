# Release gate ledger

This ledger distinguishes source/CI evidence from tests that require a real target machine. A green compile is not evidence of latency, zero xruns, or long-duration stability.

| Gate | Current evidence | Status |
|---|---|---|
| C++20 headless configure/build/contracts | GitHub Actions [run 39273263409](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39273263409), job `linux-cxx20` | Pass on tested revision; re-run after subsequent commits |
| ALSA-enabled configure/build/contracts | Same run, job `linux-alsa-enabled`; compile/contracts only | Pass on tested revision; not hardware validation |
| Qt 6 mixer configure/build/contracts | Same run, job `linux-qt6-mixer-gui` | Pass on tested revision; Qt interaction test remains open |
| Qt 6 + ALSA live mixer | Same run, job `linux-qt6-alsa-live-mixer` | Pass on tested revision; audible hardware/loopback verification remains open |
| ASan instrumented CI | Same run, job `linux-asan` | Pass on tested revision; production graph-swap integration/stress remains open |
| TSan instrumented CI | Same run, job `linux-tsan` | Pass on tested revision; production graph-swap integration/race coverage remains open |
| Repository artifact hygiene | Same run, job `repo-artifact-hygiene` | Pass on tested revision; legacy ZIP disposition still requires content review |
| GUI-to-live-engine command path | Explicit Start/Stop Live ALSA; edits/undo/redo enqueue bounded SPSC snapshots before project/history mutation; core mixer tests cover block-boundary application, queue-full rejection, invalid indices, non-finite and out-of-range values. Capture L/R feed tracks 1–2 only | Source and core contract coverage implemented; all seven jobs passed in run 39273263409; Qt interaction, loopback, CLI parity, and target-device checks open |
| GUI fader command applies at audio block boundary | `RealtimeTrackMixer` applies queued controls at process block boundaries; regression tests assert output changes and deterministic queue saturation rejection; no Qt interaction/loopback test | Core engine contract covered in run 39273263409; GUI-level evidence remains open |
| Safe graph swap and deferred reclamation | `RealtimeBusEngineHost` publishes configured snapshots off-callback; the ALSA host now routes its callback through a preallocated host adapter. A concurrent adapter/graph-replacement test is registered in CMake | Earlier engine-host tests passed in [run 39274233768](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39274233768); this branch's adapter test awaits fresh CI, and target-device validation remains open |
| Full track/bus/master routing, insert and solo/mute behavior | Offline bus primitives exist; full track-to-live-graph semantics are incomplete | Open |
| GUI meter and xrun polling | GUI polls callback/recovery counters from the ALSA backend; peak/RMS meters are not implemented | Partial; target-device verification open |
| ALSA hardware startup/shutdown and callback timing | No target-device artifact attached | Open |
| 60-minute zero-xrun run | No target-device artifact attached | Open |
| Loopback latency and underrun injection | No measurement report | Open |
| 500-track performance budget | `tools/hush_routing_benchmark.cpp` and CMake target added; no qualifying report attached | Partial; 500-track run required |
| 8-hour soak and memory-drift bound | `tools/hush_headless_soak.cpp` emits JSON counters/RSS where available; no 8-hour report or pass threshold attached | Partial; run and threshold required |
| Process-crash recovery | Completed-record restart and truncated-tail behavior have existing coverage | Partial; power-loss durability not claimed |
| Partial-write fault injection | `tests/recovery_partial_write_tests.cpp` includes interrupted-header and truncated-payload cases; CI passed these in run 39255426377 | Pass on tested revision; power-loss durability not claimed |
| Autosave lifecycle | Gate 7 review branch adds dirty-aware policy, typed save errors, journal writer injection, operation-log dirty state, GUI open/save/quit/timer hooks, CLI project-open/recovery handling, and lifecycle tests | In progress; fresh CI has not reported for this revision; Qt close-dialog and CLI dirty-quit failure tests remain open |
| Legacy ZIP disposition | Committed ZIP remains; content has not yet been reviewed in this gate ledger | Open; do not delete or relocate before review |

## Report requirements

Target-machine reports should be JSON and include at least: commit SHA, OS/kernel, CPU, audio interface and driver, sample rate, period size, duration, xrun count, latency method/results, memory measurements, test command, and pass/fail. Missing hardware metadata or an unrun gate must be marked `not_run`, never inferred as passing.

See [PHASE_1_TO_7_EXECUTION.md](PHASE_1_TO_7_EXECUTION.md) for the seven-phase completion ledger.
