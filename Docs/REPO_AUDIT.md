# Repository hygiene audit

**Audited ref:** `main` at `0c49e20b42617e905495b202b88948201fed2a8d`  
**Scope:** tracked-tree inventory and source/documentation/build-reference review. No history rewrite was performed.  
**Important limitation:** the GitHub file API returned no content for the 2.8 MB integration-starter archive. The small source ZIP can be identified as a ZIP container, but its contents have not been fully extracted and compared against the source tree. Those two archives are therefore preserved as **unknown** pending a byte-level review. The four 2-byte “ZIPs” contain only a line ending, not a ZIP local-file header, and are malformed placeholders.

## Tracked-file inventory

All tracked files at the audited ref are listed below. Build/test relevance is summarized by location: CMake sources are listed in `CMakeLists.txt`; contract tests are registered there when `HUSH_BUILD_TESTS=ON`; docs are linked from the README or are supporting design/validation records. ZIPs and the image are separately audited below.

| Tracked path | Classification / reference audit |
|---|---|
| `.github/workflows/hush-0.2.yml` | CI configuration. |
| `.gitignore` | Repository metadata/documentation. |
| `Adapter/hush-daw-codex-cosmic-studio-engine-bridge-0.2.1 (1).zip` | Archive; see archive table below. |
| `CHANGELOG.md` | Repository metadata/documentation. |
| `CMakeLists.txt` | Build and test target registry. |
| `Docs/ALSA_AUDIO_BACKEND.md` | Documentation/design/test evidence; not compiled. |
| `Docs/AUTOSAVE_CONTROLLER.md` | Documentation/design/test evidence; not compiled. |
| `Docs/BUS_RENDERER.md` | Documentation/design/test evidence; not compiled. |
| `Docs/BUS_ROUTING_FOUNDATION.md` | Documentation/design/test evidence; not compiled. |
| `Docs/DECISION_LOG.md` | Documentation/design/test evidence; not compiled. |
| `Docs/HUSH_0.3A_MIXER_GUI.md` | Documentation/design/test evidence; not compiled. |
| `Docs/IMPLEMENTATION_STATUS.md` | Documentation/design/test evidence; not compiled. |
| `Docs/PARAMETER_QUEUE.md` | Documentation/design/test evidence; not compiled. |
| `Docs/PREALLOCATED_BUS_PROCESSOR.md` | Documentation/design/test evidence; not compiled. |
| `Docs/Prompt` | Documentation/design/test evidence; not compiled. |
| `Docs/REALTIME_BUS_ENGINE.md` | Documentation/design/test evidence; not compiled. |
| `Docs/RECOVERY_JOURNAL.md` | Documentation/design/test evidence; not compiled. |
| `Docs/RECOVERY_RESTART_TEST.md` | Documentation/design/test evidence; not compiled. |
| `Docs/RELEASE_GATES.md` | Documentation/design/test evidence; not compiled. |
| `Docs/VALIDATION_REPORT.md` | Documentation/design/test evidence; not compiled. |
| `Hush.txt` | Repository metadata/documentation. |
| `LICENSE` | Repository metadata/documentation. |
| `Processor/HUSH-2.2.0-reconciled (1).zip` | Archive; see archive table below. |
| `README.md` | Project entry point and documented build commands. |
| `ROADMAP.md` | Repository metadata/documentation. |
| `Source/Adapter/A/hush-daw-codex-cosmic-studio-engine-bridge-0.2.1.zip` | Archive; see archive table below. |
| `Source/HUSH-Cosmic-Studio-0.2.1-GitHub-Source.zip` | Archive; see archive table below. |
| `Source/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine-codex-hush-0.2-headless-foundation.zip` | Archive; see archive table below. |
| `hush-daw-codex-hush-processor-integration-starter (1).zip` | Archive; see archive table below. |
| `images/1792375771560.jpg` | Image asset; no README/docs reference found. |
| `include/hush/alsa_audio_backend.hpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `include/hush/autosave_controller.hpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `include/hush/bus_graph.hpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `include/hush/bus_renderer.hpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `include/hush/mixer.hpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `include/hush/operation_log.hpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `include/hush/preallocated_bus_processor.hpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `include/hush/project.hpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `include/hush/realtime_bus_audio_adapter.hpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `include/hush/realtime_bus_engine.hpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `include/hush/recovery_journal.hpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `include/hush/spsc_parameter_queue.hpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `src/alsa_audio_backend.cpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `src/alsa_realtime_host.cpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `src/autosave_controller.cpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `src/bus_graph.cpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `src/bus_renderer.cpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `src/mixer.cpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `src/mixer_gui.cpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `src/operation_log.cpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `src/preallocated_bus_processor.cpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `src/project.cpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `src/realtime_bus_audio_adapter.cpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `src/realtime_bus_engine.cpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `src/recovery_journal.cpp` | C++ implementation/header; source integration is in CMakeLists.txt. |
| `tests/bus_graph_tests.cpp` | Test source; test target registration is in CMakeLists.txt. |
| `tests/bus_renderer_tests.cpp` | Test source; test target registration is in CMakeLists.txt. |
| `tests/contract_tests.cpp` | Test source; test target registration is in CMakeLists.txt. |
| `tests/parameter_queue_tests.cpp` | Test source; test target registration is in CMakeLists.txt. |
| `tests/preallocated_bus_processor_tests.cpp` | Test source; test target registration is in CMakeLists.txt. |
| `tests/realtime_bus_audio_adapter_tests.cpp` | Test source; test target registration is in CMakeLists.txt. |
| `tests/realtime_bus_engine_tests.cpp` | Test source; test target registration is in CMakeLists.txt. |
| `tests/recovery_restart_tests.cpp` | Test source; test target registration is in CMakeLists.txt. |

## Archive inventory

| File | Status | Action |
|---|---|---|
| `Adapter/hush-daw-codex-cosmic-studio-engine-bridge-0.2.1 (1).zip` | **Malformed** — 2 bytes; content is a line ending, not a ZIP archive | Remove in this cleanup. |
| `Processor/HUSH-2.2.0-reconciled (1).zip` | **Malformed** — 2 bytes; content is a line ending, not a ZIP archive | Remove in this cleanup. |
| `Source/Adapter/A/hush-daw-codex-cosmic-studio-engine-bridge-0.2.1.zip` | **Malformed** — 2 bytes; content is a line ending, not a ZIP archive | Remove in this cleanup. |
| `Source/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine-codex-hush-0.2-headless-foundation.zip` | **Malformed** — 2 bytes; content is a line ending, not a ZIP archive | Remove in this cleanup. |
| `Source/HUSH-Cosmic-Studio-0.2.1-GitHub-Source.zip` | **Unknown** — valid ZIP container; full contents not extracted and compared | Preserve; review contents before deciding whether to extract/delete. |
| `hush-daw-codex-hush-processor-integration-starter (1).zip` | **Unknown** — 2.8 MB; file API did not return bytes for inspection | Preserve; review contents before deciding whether to extract/delete. |

No archive is referenced by CMake or a registered test. The four malformed placeholders have no usable contents. The two unknown archives are not deleted because redundancy/uniqueness could not be established from the available byte-level evidence.

## Image inventory

| File | Status | Action |
|---|---|---|
| `images/1792375771560.jpg` | No reference found in README or tracked documentation; not used by CMake/tests | Remove from the source tree. |

## Rename-commit audit

The following historical commits are named `Rename …`. They move archive/image assets, not compiled source files. The current paths below are present in the tracked tree; none is referenced by `CMakeLists.txt`, tests, or README build commands. No historical path was rewritten.

| Commit | Current path / finding |
|---|---|
| `19f1b93b9e` | `Source/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine-codex-hush-0.2-headless-foundation.zip` — malformed placeholder; removed. |
| `a6a066e9ed` | `Source/Adapter/A/hush-daw-codex-cosmic-studio-engine-bridge-0.2.1.zip` — malformed placeholder; removed. |
| `ff90328131` | `Source/HUSH-Cosmic-Studio-0.2.1-GitHub-Source.zip` — preserved pending content comparison. |
| `43182b8d67` | Earlier archive location; current tree contains the later `Source/Adapter/A/…zip` path above, which is malformed and removed. |
| `6cddeb29f3` | `images/1792375771560.jpg` — unreferenced image; removed. |
| `799a9243f9` | `Processor/HUSH-2.2.0-reconciled (1).zip` — malformed placeholder; removed. |
| `913eba70b8` | `Adapter/hush-daw-codex-cosmic-studio-engine-bridge-0.2.1 (1).zip` — malformed placeholder; removed. |
| `598b070cb9` | Historical misspelled `Proccessor/…` location; not present in current tree. Current `Processor/…` placeholder is malformed and removed. |
| `84d649774d` | Historical root `HUSH-2.2.0-reconciled.zip` location; not present in current tree. |

## README claim audit

| Claim area | Evidence | Honest boundary |
|---|---|---|
| Schema-v1 project persistence and compatibility | `include/hush/project.hpp`, `src/project.cpp`, `tests/contract_tests.cpp` | Contract tests cover the project API; no claim of all historical project compatibility. |
| Bus graph validation | `include/hush/bus_graph.hpp`, `src/bus_graph.cpp`, `tests/bus_graph_tests.cpp` | Validation and cycle rejection only; not a live, safely swappable graph. |
| Offline renderer | `include/hush/bus_renderer.hpp`, `src/bus_renderer.cpp`, `tests/bus_renderer_tests.cpp` | Reference/offline renderer, not a complete track mixer. |
| Preallocated processing and SPSC commands | `include/hush/preallocated_bus_processor.hpp`, `src/preallocated_bus_processor.cpp`, `include/hush/spsc_parameter_queue.hpp`, `tests/preallocated_bus_processor_tests.cpp`, `tests/parameter_queue_tests.cpp` | Code avoids explicit allocation/locking/logging/I/O in the processing function; this is not a formal hard-real-time proof. |
| Queued fader changes and stereo adapter | `src/realtime_bus_engine.cpp`, `tests/realtime_bus_engine_tests.cpp`, `src/realtime_bus_audio_adapter.cpp`, `tests/realtime_bus_audio_adapter_tests.cpp` | Fader command and adapter contracts only; not full GUI binding or track-to-master routing. |
| ALSA backend/host | `src/alsa_audio_backend.cpp`, `src/alsa_realtime_host.cpp`, `Docs/ALSA_AUDIO_BACKEND.md` | CI compile/contracts do not establish real-device reliability or zero xruns. |
| Recovery journal/autosave | `src/recovery_journal.cpp`, `src/autosave_controller.cpp`, `tests/recovery_restart_tests.cpp` | Process-restart recovery and truncated-tail handling; power-loss durability and lifecycle integration remain unverified. |
| Undo/redo operation log | `src/operation_log.cpp`, `tests/contract_tests.cpp` | Primitive operation-log tests; not a full DAW edit history UI. |
| Qt mixer shell | `src/mixer_gui.cpp`, `Docs/HUSH_0.3A_MIXER_GUI.md` | Edits project state; it is not connected to live audio. |
| Performance, soak, hardware latency/xruns | No completed target-machine report/harness in the tracked tree at audit time | Explicitly unverified; owner: project maintainer; requires named Linux/ALSA target machine and attached reports. |
| Sanitizers | `.github/workflows/hush-0.2.yml` | CI jobs exist; each run’s result is evidence only for the tested configuration, not hardware/soak behavior. |

## Verification status

This audit is a repository/API inspection. A fresh local clone/build could not be run in the current execution environment because outbound DNS/network access to GitHub is unavailable. The pull-request CI run must provide the build and CTest evidence; hardware, long-soak, and archive-content verification remain separate gates.
