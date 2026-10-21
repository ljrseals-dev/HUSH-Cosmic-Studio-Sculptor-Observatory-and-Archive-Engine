# HUSH Cosmic Studio — Engine Foundation and Optional Mixer GUI

HUSH is an offline-first C++20 audio-engine foundation with an optional Qt 6 mixer shell and optional Linux ALSA host. **It is not yet a finished DAW or a release-qualified live-audio system.** The current source tree includes project persistence, bus-graph validation, an offline reference renderer, preallocated bus processing, a bounded SPSC parameter queue, stereo audio adapters, recovery/operation-log primitives, and optional host/UI components.

Recording, MIDI, plugin hosting, voice control, and complete production-grade multitrack routing are not delivered features. The optional live GUI path is a limited mixer/input-monitor integration, not a complete DAW signal path.

## Current scope at a glance

| Area | Current documented scope | Remaining qualification |
|---|---|---|
| Project model | Schema-v1 model and JSON reader/writer | Schema-v2, migration, and broader session features are not claimed |
| Engine/core | Bus-graph validation, cycle detection, offline reference rendering, preallocated bus processing, bounded SPSC parameter queue | Full production mixer signal path is incomplete |
| Audio adapters | Interleaved-stereo adapters; host adapter uses preallocated planar scratch and a callback guard | Hardware timing, xrun behavior, and stress qualification remain outstanding |
| ALSA | Optional Linux ALSA host, explicitly started | Physical-device startup/shutdown, latency, and long-duration tests are not certified |
| Qt mixer | Optional Qt 6 Widgets mixer shell; with GUI and ALSA enabled, Start Live ALSA connects supported controls to a bounded realtime track-mixer queue | Capture mapping is stereo input channels to tracks 1–2 only; complete track-to-bus/master routing, inserts, full mute/solo behavior, meters, and xrun counters are incomplete |
| Recovery | Recovery journal, process-restart and truncated-tail tests, autosave policy controller | Host lifecycle integration is incomplete; power-loss durability is not claimed |
| Privacy | Privacy contract and source-level denylist regression check | Source scanning alone cannot prove zero runtime network activity |

## Privacy and offline operation

HUSH's intended runtime contract is local-first:

- **No telemetry or analytics:** no usage tracking or analytics reporting is intended.
- **No call-home behavior:** automatic update checks, remote configuration, license pings, webhooks, and network callbacks are not intended product behavior.
- **No crash uploads or hidden capture:** diagnostics and project/audio data are not automatically uploaded; audio processing is local.
- **Explicit local file actions:** projects, audio, exports, and recovery data use local paths selected by the user. Network destinations are not a default workflow.
- **Realtime callback discipline:** audio-device callbacks must not perform network access, disk I/O, logging, memory allocation, or blocking operations.
- **Offline build:** dependencies must already be installed. CMake configuration must not download or bootstrap dependencies.

See [Docs/PRIVACY_CONTRACT.md](Docs/PRIVACY_CONTRACT.md) for the engineering contract and review checklist.

### Privacy regression check

When `HUSH_BUILD_TESTS=ON`, CTest registers `hush_privacy_source_audit`, a source-level denylist scan over `src/` and `include/`. It looks for selected network, telemetry, and crash-reporting API markers.

**Scope limitation:** this scan is a regression tripwire, not proof that the program can never use the network. It can miss indirect calls, aliases, dependencies, platform-specific code, dynamically loaded modules, or APIs not on its denylist. Release review still needs dependency inspection and runtime observation for the exact build and target machine. A green CI run does not certify zero network activity.

## Build offline

Dependencies must already be installed on the machine. These commands describe the documented build paths; **they have not been run as part of this README edit**. CI may install system build dependencies before CMake configuration, but the project build itself must not fetch or bootstrap them.

### Core and tests

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DHUSH_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

### Optional ALSA host

On Linux, install the ALSA development headers/libraries before configuring:

```sh
cmake -S . -B build-alsa -DHUSH_ENABLE_ALSA=ON -DHUSH_BUILD_TESTS=ON
cmake --build build-alsa
ctest --test-dir build-alsa --output-on-failure
./build-alsa/hush_alsa_realtime_host
```

The minimal host uses ALSA default capture/playback devices at 48 kHz and 128 frames per period. Press Ctrl-C to stop. Compilation and contract tests are not a substitute for testing a named physical audio device.

### Optional Qt mixer GUI

Install Qt 6 Widgets before configuring:

```sh
cmake -S . -B build-gui -DHUSH_BUILD_TESTS=ON -DHUSH_BUILD_GUI=ON
cmake --build build-gui --parallel 2
ctest --test-dir build-gui --output-on-failure
./build-gui/hush_mixer_gui
```

For the optional live ALSA GUI path, enable both `HUSH_BUILD_GUI=ON` and `HUSH_ENABLE_ALSA=ON` in the same build directory. The live action is explicit; it is not a background capture feature.

### Optional offline benchmark and soak harnesses

`HUSH_BUILD_STRESS_TOOLS` defaults to `ON`. The standard build therefore also creates `hush_routing_benchmark` and `hush_headless_soak`; set `-DHUSH_BUILD_STRESS_TOOLS=OFF` to omit them. Example short smoke runs:

```sh
./build/hush_routing_benchmark 500 10
./build/hush_headless_soak 60 ./hush-soak.journal
```

The benchmark emits JSON with synthetic track-mixer throughput and rejected parameter-update counts. The headless soak runs a synthetic 500-track mixer and periodically writes recovery snapshots. These are offline software harnesses, not audio-device tests; the program output explicitly reports `"hardware_audio_test": false` and does not apply a pass threshold. CI smoke-runs each harness briefly, which is not equivalent to a 500-track performance qualification, an eight-hour soak, or a hardware zero-xrun test. Use a disposable journal path and remove it after the run if it is no longer needed.

## Included source and test references

- **Schema-v1 project model and JSON reader/writer:** `include/hush/project.hpp`, `src/project.cpp`; contract coverage in `tests/contract_tests.cpp`.
- **Bus-graph validation and cycle detection:** `include/hush/bus_graph.hpp`, `src/bus_graph.cpp`, `tests/bus_graph_tests.cpp`.
- **Offline/reference bus-buffer renderer:** `include/hush/bus_renderer.hpp`, `src/bus_renderer.cpp`, `tests/bus_renderer_tests.cpp`.
- **Preallocated bus processing:** `include/hush/preallocated_bus_processor.hpp`, `src/preallocated_bus_processor.cpp`, `tests/preallocated_bus_processor_tests.cpp`.
- **Bounded SPSC parameter queue:** `include/hush/spsc_parameter_queue.hpp`, `tests/parameter_queue_tests.cpp`.
- **Block-boundary fader updates:** `include/hush/realtime_bus_engine.hpp`, `src/realtime_bus_engine.cpp`, `tests/realtime_bus_engine_tests.cpp`.
- **Interleaved stereo adapters:** `include/hush/realtime_bus_audio_adapter.hpp` for the configure-once engine and `include/hush/realtime_bus_engine_host_audio_adapter.hpp` for the published-graph host. The host adapter preallocates planar scratch, holds the graph host's callback guard through processing/output copy, and documents same-topology replacement while running. `tests/realtime_bus_engine_host_audio_adapter_tests.cpp` covers output mapping and concurrent graph replacement. The latest adapter change still needs fresh CI evidence before it is described as CI-verified.
- **Recovery journal and restart test:** `include/hush/recovery_journal.hpp`, `src/recovery_journal.cpp`, `tests/recovery_restart_tests.cpp`; policy controller in `src/autosave_controller.cpp`.
- **Operation-log undo/redo primitives:** `include/hush/operation_log.hpp`, `src/operation_log.cpp`, `tests/contract_tests.cpp`.
- **Optional ALSA backend/host:** `src/alsa_audio_backend.cpp`, `src/alsa_realtime_host.cpp`; see `Docs/ALSA_AUDIO_BACKEND.md`.
- **Optional Qt 6 mixer GUI:** `src/mixer_gui.cpp`; see `Docs/HUSH_0.3A_MIXER_GUI.md`.
- **Privacy contract/source audit:** `Docs/PRIVACY_CONTRACT.md`, `tools/privacy_source_audit.cmake`, registered by `CMakeLists.txt`.
- **Build/test registration and CI configuration:** `CMakeLists.txt`, `.github/workflows/hush-0.2.yml`.

The source references above identify intended code/test locations; they do not independently establish that every target has been built or every test has passed on the current commit.

## Outstanding implementation and release gates

The following are **not complete or not verified**. A claim should be upgraded only when reproducible evidence for the exact commit and target configuration is committed.

- **GUI-to-live-engine automated interaction and GUI/CLI parity:** supported GUI edits and undo/redo are queued to the live track-mixer path, but automated Qt interaction/loopback coverage and target-device validation remain outstanding.
- **Graph replacement and deferred reclamation:** the host adapter documents guarded same-topology replacement while processing. That does not establish complete general graph-swap/reclamation safety; dedicated lifetime/reclamation stress coverage, including ASan/TSan evidence, remains outstanding.
- **Complete mixer signal path:** the ALSA CLI host now uses persisted project bus-to-bus routes when present (otherwise the default capture→master graph). Track-source assignment into buses, track controls in that path, inserts, end-to-end mute/solo, graph-wide meters, and integrated xrun reporting are still not implemented as a complete live mixer.
- **ALSA hardware behavior:** real-device startup/shutdown, callback timing, lock-free suitability on target atomics, and xrun reporting remain unverified for a named Linux kernel/CPU/ALSA/device configuration.
- **Long-duration audio qualification:** no committed 60-minute zero-xrun report, loopback-latency report, or underrun-injection report is claimed.
- **Scale and soak qualification:** no committed 500-track performance budget or 8-hour soak/memory-drift report is claimed.
- **Autosave lifecycle/failure surfacing:** the ALSA CLI host and Qt GUI now poll the recovery-journal controller off the audio callback. The GUI prompts before restoring a newer snapshot, honors explicit discard by clearing the journal, and surfaces/retries journal failures. Automated Qt recovery/autosave interaction coverage and host-level failure-path coverage remain open.
- **Power-loss durability:** explicitly not claimed. Current documented evidence covers completed journal records across process restart and truncated-tail handling only.
- **MIDI/automation integration:** this branch adds a bounded MIDI CC-to-parameter mapping foundation. MIDI device I/O, GUI learn mode, engine parameter delivery, automation playback, and schema-v2 persistence remain unimplemented.
- **Archive contents:** two non-empty archives remain preserved but unverified; see `Docs/REPO_AUDIT.md`.

## CI and evidence policy

CI status is commit-specific. A successful run for an earlier commit is historical evidence for that revision, not proof for the current branch head. For every release candidate, record the exact commit SHA, workflow/run URL, configuration, test outcome, and any skipped jobs. Keep these categories separate:

1. **Automated CI:** build/test/sanitizer results for a named commit and configuration.
2. **Local validation:** commands actually run, environment, and results.
3. **Hardware validation:** named machine, kernel, CPU, audio interface/driver, sample rate, buffer size, duration, xrun count, and measured latency.
4. **Not tested:** anything without recorded evidence.

Do not claim hardware readiness, performance, soak stability, release readiness, or a verified zero-network guarantee from green CI alone.

## Project status and handoff

See [Docs/IMPLEMENTATION_STATUS.md](Docs/IMPLEMENTATION_STATUS.md), [Docs/RELEASE_GATES.md](Docs/RELEASE_GATES.md), [Docs/REPO_AUDIT.md](Docs/REPO_AUDIT.md), [Docs/PRIVACY_CONTRACT.md](Docs/PRIVACY_CONTRACT.md), and [ROADMAP.md](ROADMAP.md).

- [Master Builder Prompt — Open All Implementation Gates](Docs/MASTER_BUILDER_PROMPT.md) defines an implementation sequence while preserving the privacy contract and requiring evidence for claimed results.
- [PR #21 — repository audit and baseline](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/21) is the repository audit/baseline handoff.
- [PR #22 — privacy source-contract regression check](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/22) contains privacy-contract changes; check its current status before treating it as merged.
- The optional Qt mixer and ALSA host are foundations for further integration, not a finished DAW. Keep live audio opt-in, preserve the realtime callback constraints, and do not claim unmeasured behavior.
