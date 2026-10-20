# Repository audit — HUSH Cosmic Studio

**Audited ref:** `main`, commit/tree `9eb408c3d8cc004898fbdd16e9b901c1853b76d9`  
**Audit date:** 2026-10-20  
**Method:** GitHub tracked-tree, source, CMake, pull-request, and Actions inspection. This is not a local checkout build. No source files or archives were deleted by this audit.

## Current inventory

The recursive Git tree is not truncated and contains **113 tracked blobs** across 9 tree entries. The main source layout is:

- `src/`: 23 C/C++ source files, including the core, optional ALSA host/backend, and optional Qt mixer.
- `include/hush/`: 24 public headers.
- `tests/`: 22 C++ contract-test sources.
- `tools/`: routing benchmark, voice-path benchmark, headless soak tool, and the privacy source-audit CMake script.
- `Docs/`: implementation notes, build/evidence logs, release gates, privacy, routing, recovery, media, GUI, and audit documents.
- Root: `CMakeLists.txt`, README, roadmap, changelog, contributing/security/license files, and `Hush.txt`.

No tracked image/audio files or build output directories were found in the recursive tree. The two ZIP archives below are the only tracked binary archives identified.

## Build configuration and targets

CMake minimum is 3.20; project `HushUnifiedDAW` is C++20 with extensions disabled.

| Option | Default | Effect |
|---|---:|---|
| `HUSH_BUILD_TESTS` | ON | Registers privacy source audit and 22 offline C++ contract executables. |
| `HUSH_ENABLE_ALSA` | OFF | Linux-only; requires preinstalled ALSA headers/libraries. Builds `hush_alsa_backend` and `hush_alsa_realtime_host`. |
| `HUSH_BUILD_GUI` | OFF | Requires preinstalled Qt 6 Widgets; builds `hush_mixer_gui`. |
| `HUSH_BUILD_STRESS_TOOLS` | ON | Builds routing benchmark, voice-path benchmark, and headless soak executable. |

The always-built `hush_core` currently includes project/operation-log/mixer code, bus graph and offline renderer, preallocated and realtime bus/track processors, graph host and audio adapters, transport/automation primitives, MIDI learn mapping, mixer engine bridge, recovery journal/autosave, WAV reader/writer, and project WAV export.

Registered CTest executable targets are:

- `hush_contract_tests`, `hush_bus_graph_tests`, `hush_bus_renderer_tests`
- `hush_preallocated_bus_processor_tests`, `hush_realtime_bus_engine_tests`, `hush_realtime_bus_engine_host_tests`
- `hush_realtime_bus_audio_adapter_tests`, `hush_realtime_bus_engine_host_audio_adapter_tests`
- `hush_mixer_engine_bridge_tests`, `hush_realtime_track_mixer_tests`
- `hush_transport_automation_tests`, `hush_midi_learn_tests`
- `hush_realtime_graph_publisher_tests`, `hush_parameter_queue_tests`
- `hush_recovery_restart_tests`, `hush_recovery_partial_write_tests`, `hush_autosave_lifecycle_tests`
- `hush_project_close_policy_tests`, `hush_wav_writer_tests`, `hush_project_wav_export_tests`
- `hush_project_media_timeline_tests`, `hush_wav_reader_tests`
- Plus the `hush_privacy_source_audit` CTest script.

Assert-based contract tests explicitly undefine `NDEBUG` in Release builds. No source globbing, `FetchContent`, configure-time network fetch, or package-bootstrap script was found in `CMakeLists.txt`. Optional system dependencies must be installed before configuration; CI installs them on its own runner.

## Archive inventory — preserve pending byte-level review

| File | Size | Classification | Action |
|---|---:|---|---|
| `Source/HUSH-Cosmic-Studio-0.2.1-GitHub-Source.zip` | 9,508 bytes | Unknown; contents not extracted and compared in this audit | Preserve; extract and compare before deciding if redundant. |
| `hush-daw-codex-hush-processor-integration-starter (1).zip` | 2,795,731 bytes | Unknown; binary contents not available through repository text inspection | Preserve; inspect provenance and unique files before disposition. |

Neither archive is referenced by CMake or tests. That does not prove its contents are redundant. CI temporarily allowlists these two legacy archives and rejects new or modified ZIPs. Remove the allowlist only after the archives are dispositioned. Do not delete either archive solely from its filename or size.

## Pull-request and CI audit snapshot

This is a source-and-CI status snapshot, not a claim that every PR has been merged:

| PR | Finding / disposition | Validation boundary |
|---|---|---|
| [#17 mixer gain bridge](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/17) | Closed, unmerged; the bridge and tests are now represented on `main` through later commits. | Does not by itself close live GUI/host integration. |
| [#23 transport/tempo/automation core](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/23) | Closed, unmerged; canonical transport and automation primitives are on `main`, with subsequent regression fixes. | No claim of complete GUI/audio callback or persistence integration. |
| [#43 WAV media import](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/43) | Closed without merge as stale/superseded; WAV reader and tests are already on `main`. | Import is not full arrangement playback. |
| [#54 final-save coverage](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/54) | Closed without merge as stale; final-save contracts were ported to #64 and CLI recovery handling to #66. | The older branch is not a valid current-main baseline. |
| [#62 tempo/automation persistence](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/62) | Open; audit fixes include escaped-string round-trip support across all project string fields, a valid automation-array capture, and strict numeric-token parsing. | Exact current head passed all seven jobs in [run 39370045257](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39370045257); not merged into `main`. |
| [#63 graph-publication stress-test race](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/63) | Open; adds a startup handshake so the control thread cannot stop the test before the audio thread processes one block. | All seven jobs passed in [run 39368577592](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39368577592); not merged into `main`. |
| [#64 host final-save contracts](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/64) | Open; clean port to current `main` of final-save helper/tests, including writer-exception handling. | All seven jobs passed in [run 39369444789](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39369444789); not merged into `main`. |
| [#66 ALSA host recovery errors](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/66) | Open; ports CLI project/recovery handling to current `main` and surfaces corrupt/unreadable journal errors before audio startup. | All seven jobs passed in [run 39370447074](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39370447074); project metadata is still not live routing. |

Current-main CI for commit `9eb408c3d8cc004898fbdd16e9b901c1853b76d9` passed all seven jobs in [run 39361037813](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39361037813): default C++20 contracts and smoke tools, ALSA-enabled build, Qt GUI build, Qt+ALSA build, ASan, TSan, and repository artifact hygiene. A prior run failed because a graph-publication stress test could finish before its audio thread was scheduled; the follow-up test fix is in PR #63 and passed CI.

## README claim-to-evidence boundaries

| Area | Evidence | Limit |
|---|---|---|
| Schema-v1 persistence | `project.hpp`, `project.cpp`, contract tests | Does not establish compatibility with every historical external session. |
| Bus graph and offline rendering | `bus_graph.cpp`, `bus_renderer.cpp`, tests | Not a full live DAW signal path. |
| Realtime queues/processors | SPSC queue, bus/track engines and contract tests | Not a formal hard-real-time proof; target atomic lock-free behavior must be measured. |
| Graph publication | Publisher and concurrent tests | Lifecycle primitive coverage is not a target-device audio qualification. |
| WAV import/export | WAV reader/writer and project export tests | Import/export foundations do not provide full media arrangement playback or resampling. |
| Recovery/autosave | Journal, controller, restart/partial-write/lifecycle tests | Does not prove power-loss durability; final-save lifecycle helper is proposed in PR #64, not on `main`. |
| Optional ALSA host | Backend, host, adapter, CI | CI does not prove physical-device startup/shutdown, latency, or zero xruns. |
| Optional Qt GUI | Mixer GUI and Qt CI | GUI integration/loopback coverage and complete GUI/CLI parity remain open. |
| Benchmarks and soak | Offline tools and CI smoke runs | Smoke runs are not a qualifying 500-track budget or eight-hour soak. |
| Privacy | Source denylist test and privacy contract | Source scan is not a formal proof of zero network activity at runtime. |

## Build commands and evidence policy

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DHUSH_BUILD_TESTS=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure

cmake -S . -B build-gui -DHUSH_BUILD_TESTS=ON -DHUSH_BUILD_GUI=ON
cmake --build build-gui --parallel 2
ctest --test-dir build-gui --output-on-failure

cmake -S . -B build-alsa -DHUSH_BUILD_TESTS=ON -DHUSH_ENABLE_ALSA=ON
cmake --build build-alsa --parallel 2
ctest --test-dir build-alsa --output-on-failure
```

**Local build status: not run in this audit workflow.** GitHub Actions results above are the evidence for the exact listed commits. Do not claim that a local checkout build was run here. Hardware startup/shutdown, measured latency, xrun/loopback qualification, large-project performance, long soak, full live automation playback, and byte-level archive inspection remain open gates.

## Cleanup decision

- Preserve both ZIP archives until contents and provenance are verified.
- Keep the existing artifact ignore rules and reject newly added build outputs/archives.
- Do not rewrite published history.
- Keep the distinction between source presence, test coverage, CI success, and full DAW integration explicit.
