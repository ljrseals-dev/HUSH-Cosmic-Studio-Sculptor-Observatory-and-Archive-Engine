# HUSH Cosmic Studio — 0.2 headless foundation

HUSH is an offline-first C++20 audio-engine foundation, not a finished DAW release. The current tree contains project persistence, bus-graph validation, an offline reference renderer, preallocated bus processing, a bounded SPSC parameter queue, a stereo audio adapter, an optional ALSA host, an optional Qt 6 mixer shell, and recovery/operation-log primitives. Recording, MIDI, plugin hosting, and voice control remain out of scope.

## Privacy and offline operation

HUSH's intended runtime contract is local-first:

- **No telemetry or analytics:** no usage tracking or analytics reporting.
- **No call-home behavior:** no automatic update checks, remote configuration, license pings, webhooks, or network callbacks.
- **No crash uploads or hidden capture:** diagnostics and project/audio data are not automatically uploaded; audio is processed locally.
- **Explicit local file actions:** projects, audio, exports, and recovery data are stored at local paths selected by the user. Network destinations are not a default workflow.
- **Realtime callback discipline:** audio-device callbacks are expected to process audio without network access, disk I/O, logging, memory allocation, or blocking operations.
- **Offline build:** dependencies must already be installed; the project does not download or bootstrap dependencies during CMake configuration.

See [Docs/PRIVACY_CONTRACT.md](Docs/PRIVACY_CONTRACT.md) for the engineering contract and review checklist.

### Privacy regression check

When HUSH_BUILD_TESTS=ON, CTest includes `hush_privacy_source_audit`, a source-level denylist scan over `src/` and `include/`. It looks for selected network, telemetry, and crash-reporting API markers.

**Scope limitation:** this scan is a regression tripwire, not proof that the program can never use the network. It can miss indirect calls, aliases, dependencies, platform-specific code, dynamically loaded modules, or APIs not on its denylist. Release review still needs dependency inspection and runtime observation with the relevant build and target machine. A green CI run does not certify zero network activity.

## Build offline

Dependencies must already be installed on the machine. The project build does not download or bootstrap dependencies.

\`\`\`sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DHUSH_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
\`\`\`

No FetchContent, package bootstrap, telemetry, update checks, or runtime network requests are intended parts of the CMake/runtime design. CI may install system build dependencies before configuration.

## Included — source and test evidence

- **Schema-v1 project model and JSON reader/writer:** `include/hush/project.hpp`, `src/project.cpp`; contract coverage in `tests/contract_tests.cpp`.
- **Bus graph validation and cycle detection:** `include/hush/bus_graph.hpp`, `src/bus_graph.cpp`, `tests/bus_graph_tests.cpp`.
- **Offline/reference bus-buffer renderer:** `include/hush/bus_renderer.hpp`, `src/bus_renderer.cpp`, `tests/bus_renderer_tests.cpp`.
- **Preallocated bus processing:** `include/hush/preallocated_bus_processor.hpp`, `src/preallocated_bus_processor.cpp`, `tests/preallocated_bus_processor_tests.cpp`.
- **Bounded SPSC parameter queue:** `include/hush/spsc_parameter_queue.hpp`, `tests/parameter_queue_tests.cpp`.
- **Block-boundary fader updates:** `include/hush/realtime_bus_engine.hpp`, `src/realtime_bus_engine.cpp`, `tests/realtime_bus_engine_tests.cpp`.
- **Interleaved stereo adapter:** `include/hush/realtime_bus_audio_adapter.hpp`, `src/realtime_bus_audio_adapter.cpp`, `tests/realtime_bus_audio_adapter_tests.cpp`.
- **Recovery journal and restart test:** `include/hush/recovery_journal.hpp`, `src/recovery_journal.cpp`, `tests/recovery_restart_tests.cpp`; policy controller in `src/autosave_controller.cpp`.
- **Operation-log undo/redo primitives:** `include/hush/operation_log.hpp`, `src/operation_log.cpp`, `tests/contract_tests.cpp`.
- **Optional ALSA backend/host:** `src/alsa_audio_backend.cpp`, `src/alsa_realtime_host.cpp`, documented in `Docs/ALSA_AUDIO_BACKEND.md`.
- **Optional Qt 6 project mixer shell:** `src/mixer_gui.cpp`, documented in `Docs/HUSH_0.3A_MIXER_GUI.md`. It edits project state only; it is not connected to live audio.
- **Privacy contract and source audit:** `Docs/PRIVACY_CONTRACT.md`, `tools/privacy_source_audit.cmake`, registered by `CMakeLists.txt`.
- **Build/test registration:** `CMakeLists.txt`; CI configurations and sanitizer jobs: `.github/workflows/hush-0.2.yml`.

## Optional ALSA host

With Linux ALSA development headers/libraries already installed:

\`\`\`sh
cmake -S . -B build-alsa -DHUSH_ENABLE_ALSA=ON -DHUSH_BUILD_TESTS=ON
cmake --build build-alsa
ctest --test-dir build-alsa --output-on-failure
./build-alsa/hush_alsa_realtime_host
\`\`\`

The minimal host uses ALSA default capture/playback devices at 48 kHz and 128 frames per period. Press Ctrl-C to stop. Compilation and contract tests are not a substitute for testing on a named physical audio device.

## Optional mixer GUI

With Qt 6 Widgets already installed:

\`\`\`sh
cmake -S . -B build-gui -DHUSH_BUILD_TESTS=ON -DHUSH_BUILD_GUI=ON
cmake --build build-gui --parallel 2
ctest --test-dir build-gui --output-on-failure
./build-gui/hush_mixer_gui
\`\`\`

## Not yet complete — do not claim as delivered

Every item below is **unverified/incomplete** until its evidence is committed. **Owner:** project maintainer. **Target-machine requirements** apply where specified.

- **GUI-to-live-engine commands and GUI interaction/CLI parity:** no complete GUI binding test; requires the Qt GUI and engine running together.
- **Safe graph replacement/deferred reclamation:** no graph swap implementation or reclamation stress test; requires ASan/TSan and a target Linux build.
- **Full track-to-bus-to-master routing, inserts, end-to-end mute/solo, meters, and xrun counters:** not implemented as a complete live mixer; requires unit/integration tests and target audio-device tests.
- **ALSA real-device startup/shutdown, callback timing, atomic lock-free suitability, and xrun reporting:** unverified; requires a named Linux kernel/CPU/ALSA/device configuration and captured logs.
- **60-minute zero-xrun test, loopback latency, and underrun injection:** no completed report; requires a physical/reference ALSA machine and machine-labelled results.
- **500-track performance budget and 8-hour soak/memory-drift report:** no completed report; requires a named machine, reproducible harness, and committed results.
- **Autosave lifecycle/failure surfacing:** policy code exists, but host-application lifecycle integration is incomplete.
- **Power-loss durability:** explicitly not claimed. Current evidence covers completed journal records across process restart and truncated-tail handling only.
- **Transport, MIDI, and automation:** proposed, not implemented; see [Docs/PROPOSAL_TRANSPORT_MIDI_AUTOMATION.md](Docs/PROPOSAL_TRANSPORT_MIDI_AUTOMATION.md).
- **Archive contents:** two non-empty archives remain preserved but unverified; see `Docs/REPO_AUDIT.md`.

See [Docs/IMPLEMENTATION_STATUS.md](Docs/IMPLEMENTATION_STATUS.md), [Docs/RELEASE_GATES.md](Docs/RELEASE_GATES.md), [Docs/REPO_AUDIT.md](Docs/REPO_AUDIT.md), and [ROADMAP.md](ROADMAP.md). Do not infer hardware readiness, performance, soak stability, release readiness, or a verified zero-network guarantee from green CI alone.

## Current engineering handoff

- [Master Builder Prompt — Open All Implementation Gates](Docs/MASTER_BUILDER_PROMPT.md) authorizes the implementation sequence without repeated approval pauses, while preserving the privacy contract and requiring evidence for every claimed result.

- [PR #21 — repository audit and baseline](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/21) is merged.
- [PR #22 — privacy source-contract regression check](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/22) contains the privacy-contract changes and remains a draft pending review.
- [Privacy contract](Docs/PRIVACY_CONTRACT.md) defines the intended offline/local-first behavior; the source audit is only a regression check, not proof of zero runtime network activity.
- The Qt mixer shell is **not yet connected to live audio**. Do not describe this repository as a finished DAW or claim hardware validation, long-duration stability, or a verified release build without committed evidence.

