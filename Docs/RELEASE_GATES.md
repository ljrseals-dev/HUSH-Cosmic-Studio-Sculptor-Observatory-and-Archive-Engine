# Release gate ledger

This ledger distinguishes source/CI evidence from tests that require a real target machine. A green compile is not evidence of latency, zero xruns, or long-duration stability.

| Gate | Current evidence | Status |
|---|---|---|
| C++20 headless configure/build/contracts | GitHub Actions `linux-cxx20`; run [39223892261](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39223892261) | CI pass |
| ALSA-enabled configure/build/contracts | GitHub Actions `linux-alsa-enabled`; compile/contracts only | CI pass; not hardware validation |
| Qt 6 mixer configure/build/contracts | GitHub Actions `linux-qt6-mixer-gui`; no Qt interaction test yet | CI pass; GUI-to-engine behavior open |
| ASan instrumented CI | GitHub Actions `linux-asan`; run [39223892261](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39223892261) | CI pass; does not replace graph-swap stress coverage |
| TSan instrumented CI | GitHub Actions `linux-tsan`; run [39223892261](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39223892261) | CI pass; graph-swap race coverage remains open |
| Repository artifact hygiene | GitHub Actions `repo-artifact-hygiene`; run [39223892261](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39223892261) | CI pass; legacy ZIP disposition still requires content review |
| GUI-to-live-engine command path | GUI currently edits project model only | Open |
| GUI fader command applies at audio block boundary | No GUI-to-engine integration test | Open |
| Safe graph swap and deferred reclamation | Configure-once graph; no swap/reclamation implementation | Open |
| Full track/bus/master routing, insert and solo/mute behavior | Offline bus primitives exist; full track-to-live-graph semantics are incomplete | Open |
| GUI meter and xrun polling | Not implemented | Open |
| ALSA hardware startup/shutdown and callback timing | No target-device artifact attached | Open |
| 60-minute zero-xrun run | No target-device artifact attached | Open |
| Loopback latency and underrun injection | No measurement report | Open |
| 500-track performance budget | No benchmark report | Open |
| 8-hour soak and memory-drift bound | No soak report | Open |
| Process-crash recovery | Completed-record restart and truncated-tail behavior have existing coverage | Partial; power-loss durability not claimed |
| Partial-write fault injection | No explicit fault-injection artifact identified | Open |

## Report requirements

Target-machine reports should be JSON and include at least: commit SHA, OS/kernel, CPU, audio interface and driver, sample rate, period size, duration, xrun count, latency method/results, memory measurements, test command, and pass/fail. Missing hardware metadata or an unrun gate must be marked `not_run`, never inferred as passing.

See [PHASE_1_TO_7_EXECUTION.md](PHASE_1_TO_7_EXECUTION.md) for the seven-phase completion ledger and conclusion.
