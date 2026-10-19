# Repository audit — HUSH Cosmic Studio

**Audited ref:** `main`, tree `31578b352a5b62f4a62af87745b3615be55ca10b`  
**Method:** GitHub tracked-tree and source-file inspection. This is not a local checkout build. No files were deleted by this audit.

## CMake inventory

CMake minimum: 3.20. Project: `HushUnifiedDAW` 0.2.0. Language: C++20, extensions disabled.

### Options

| Option | Default | Effect / dependency |
|---|---:|---|
| `HUSH_BUILD_TESTS` | ON | Registers CTest executables listed below. `Threads` is required for graph publisher tests. |
| `HUSH_ENABLE_ALSA` | OFF | Linux-only; requires preinstalled ALSA development headers/libraries via `find_package(ALSA REQUIRED)`. Builds `hush_alsa_backend` and `hush_alsa_realtime_host`. |
| `HUSH_BUILD_GUI` | OFF | Requires preinstalled Qt 6 Widgets; builds `hush_mixer_gui`. |
| `HUSH_BUILD_STRESS_TOOLS` | ON | Builds offline `hush_routing_benchmark` and `hush_headless_soak`. |

### Targets

| Target | Backing source(s) | Condition |
|---|---|---|
| `hush_core` | `src/project.cpp`, `operation_log.cpp`, `mixer.cpp`, `bus_graph.cpp`, `bus_renderer.cpp`, `preallocated_bus_processor.cpp`, `realtime_bus_engine.cpp`, `realtime_track_mixer.cpp`, `realtime_bus_audio_adapter.cpp`, `recovery_journal.cpp`, `autosave_controller.cpp` | Always |
| `hush_mixer_gui` | `src/mixer_gui.cpp`, links `hush_core` and `Qt6::Widgets` | `HUSH_BUILD_GUI` |
| `hush_alsa_backend` | `src/alsa_audio_backend.cpp` | `HUSH_ENABLE_ALSA` |
| `hush_alsa_realtime_host` | `src/alsa_realtime_host.cpp`, links core and ALSA backend | `HUSH_ENABLE_ALSA` |
| `hush_routing_benchmark` | `tools/hush_routing_benchmark.cpp` | `HUSH_BUILD_STRESS_TOOLS` |
| `hush_headless_soak` | `tools/hush_headless_soak.cpp` | `HUSH_BUILD_STRESS_TOOLS` |

Registered CTest executables: `hush_contract_tests`, `hush_bus_graph_tests`, `hush_bus_renderer_tests`, `hush_preallocated_bus_processor_tests`, `hush_realtime_bus_engine_tests`, `hush_realtime_bus_audio_adapter_tests`, `hush_realtime_track_mixer_tests`, `hush_realtime_graph_publisher_tests`, `hush_parameter_queue_tests`, `hush_recovery_restart_tests`, `hush_recovery_partial_write_tests`.

No source globbing, `FetchContent`, download command, package bootstrap, or configure-time source fetch was found in `CMakeLists.txt`. Optional dependencies must be installed before configuring. GitHub Actions installs system packages on its runner; that is CI provisioning, not a CMake build-time download.

## Tracked-tree inventory

At the audited tree there are 71 tracked blobs, no tracked build directories/CMake caches, and no tracked image files. See the [main tree](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/tree/main).

The tracked C++ sources under `src/`, public headers under `include/hush/`, tests under `tests/`, and stress tools under `tools/` are represented in CMake or test registration. Documentation files are supporting design, usage, audit, and release-gate material; not every design note needs a CMake target.

## Archive inventory — preserve pending byte-level review

| File | Size | Classification | Action |
|---|---:|---|---|
| `Source/HUSH-Cosmic-Studio-0.2.1-GitHub-Source.zip` | 9,508 bytes | **Unknown** — contents not extracted and compared in this audit | Preserve; download/extract and compare before deciding if redundant. |
| `hush-daw-codex-hush-processor-integration-starter (1).zip` | 2,795,731 bytes | **Unknown** — binary contents unavailable through repository text inspection | Preserve; inspect locally, document provenance/unique files, then decide whether to extract or move to Releases. |

Neither archive is referenced by CMake, tests, or README build commands. That alone does not prove its contents are redundant. No deletion is justified by current evidence. The workflow temporarily tolerates these two legacy tracked archives but rejects newly added/modified ZIPs. After the archives are dispositioned, remove the allowlist so CI rejects all tracked ZIPs.

## README claim-to-evidence audit

| Claim | Backing files/tests | Boundary |
|---|---|---|
| Schema-v1 project persistence | `include/hush/project.hpp`, `src/project.cpp`, `tests/contract_tests.cpp` | Does not prove compatibility with every historical external project. |
| Bus graph validation/cycle detection | `include/hush/bus_graph.hpp`, `src/bus_graph.cpp`, `tests/bus_graph_tests.cpp` | Not a live graph swap. |
| Offline/reference renderer | `include/hush/bus_renderer.hpp`, `src/bus_renderer.cpp`, `tests/bus_renderer_tests.cpp` | Not a complete media-playback DAW. |
| Preallocated processing and parameter queue | `include/hush/preallocated_bus_processor.hpp`, `src/preallocated_bus_processor.cpp`, `include/hush/spsc_parameter_queue.hpp`, related CTest targets | Not a formal hard-real-time proof. |
| Block-boundary fader updates/stereo adapter | `src/realtime_bus_engine.cpp`, `src/realtime_bus_audio_adapter.cpp`, respective tests | Not full GUI binding or complete track-to-master routing. |
| Realtime track-mixer primitive | `include/hush/realtime_track_mixer.hpp`, `src/realtime_track_mixer.cpp`, `tests/realtime_track_mixer_tests.cpp` | GUI is not connected to this primitive. |
| Graph-publication primitive | `include/hush/realtime_graph_publisher.hpp`, `tests/realtime_graph_publisher_tests.cpp` | Not integrated into production engine lifecycle. |
| Recovery journal and partial-write cases | `src/recovery_journal.cpp`, `tests/recovery_restart_tests.cpp`, `tests/recovery_partial_write_tests.cpp` | Does not establish power-loss durability. |
| Optional ALSA host | `src/alsa_audio_backend.cpp`, `src/alsa_realtime_host.cpp`, `Docs/ALSA_AUDIO_BACKEND.md` | CI compile/tests do not prove hardware reliability, latency, or zero xruns. |
| Optional Qt GUI | `src/mixer_gui.cpp`, `Docs/HUSH_0.3A_MIXER_GUI.md` | Project-state editor only; not connected to live audio. |
| Benchmark/soak harnesses | `tools/hush_routing_benchmark.cpp`, `tools/hush_headless_soak.cpp` | CI smoke tests do not count as a qualifying 500-track run or eight-hour soak report. |

## Build commands and evidence

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DHUSH_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure

cmake -S . -B build-gui -DHUSH_BUILD_TESTS=ON -DHUSH_BUILD_GUI=ON
cmake --build build-gui --parallel 2

cmake -S . -B build-alsa -DHUSH_ENABLE_ALSA=ON -DHUSH_BUILD_TESTS=ON
cmake --build build-alsa
```

**Local build status for this audit: not run.** Repository file inspection cannot honestly provide verbatim compiler output from a local checkout. The GitHub Actions workflow defines default, Qt, ALSA, ASan, TSan, and artifact-hygiene jobs; use the PR's actual workflow run for pass/fail evidence. Hardware tests, 60-minute stability, latency, 500-track performance, eight-hour soak, and archive-content verification remain open gates.

## Cleanup decision

- Keep the existing `.gitignore`; it already covers build trees, CMake artifacts, binaries, IDE folders, OS cruft, local audio artifacts, and ZIPs.
- Do not delete either remaining archive until its contents are reviewed.
- No build outputs or images are present in the tracked tree.
- Do not rewrite published history.
