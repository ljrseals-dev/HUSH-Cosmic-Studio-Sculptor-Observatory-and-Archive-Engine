# Gate 3 build log

## Baseline status — blocked before implementation

Requested baseline commands (verbatim):

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DHUSH_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
cmake -S . -B build-alsa -DHUSH_ENABLE_ALSA=ON -DHUSH_BUILD_TESTS=ON
cmake --build build-alsa && ctest --test-dir build-alsa --output-on-failure
cmake -S . -B build-gui -DHUSH_BUILD_TESTS=ON -DHUSH_BUILD_GUI=ON -DHUSH_ENABLE_ALSA=ON
cmake --build build-gui --parallel 2 && ctest --test-dir build-gui --output-on-failure
```

**Execution result:** NOT RUN in this work session. The available GitHub connection can inspect and commit repository files and read GitHub Actions, but it does not expose a terminal in a local clone. No command output was captured, so there is no verbatim output to paste here. This is an environment limitation, not a claim that the builds pass or fail.

**Existing CI evidence (not a substitute for the requested local baseline):** GitHub Actions run [39255426377](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39255426377) passed the seven jobs on its tested revision. Any new commit must run CI again. This historical CI result does not establish the three requested local build configurations for the current Gate 3 branch.

## Audit finding / stop condition

`Docs/ROUTING_GATE_AUDIT.md` records the source-level findings. The current code has a bus-only `BusGraph` and a separate `RealtimeTrackMixer`; the bus graph has no track nodes or track-to-bus assignments. Track-to-bus assignments and persisted insert chains require a schema-v2 change because `src/project.cpp` currently requires schema version 1. The ALSA backend already has an atomic `xrun_count()`, but the inspected implementation has no injected ALSA write stub for deterministic failure testing.

**Gate 3 implementation is paused at baseline evidence.** Run the commands above in the repository clone and append the actual terminal output before proceeding to graph/schema changes. Do not represent the gate as closed until default, ALSA, and GUI+ALSA builds and tests are green. Hardware validation, zero-xrun, latency, soak, and power-loss durability remain unclaimed.
