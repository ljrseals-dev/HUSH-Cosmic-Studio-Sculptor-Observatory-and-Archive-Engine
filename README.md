# HUSH Unified DAW — 0.2 headless foundation

This branch begins the ordered milestone sequence at **Prompt 1 — HUSH 0.2** in `Docs/Prompt`. Milestone 0.2 is headless only: persistent project state, an operation log, mixer semantics, and the foundation for real-time Linux audio and bus routing. GUI, recording, MIDI, plugins, and voice control are explicitly out of scope.

## Build offline

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DHUSH_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

No FetchContent, package bootstrap, network access, telemetry, or update checks are used by this build.

## Included
- C++20 project model with stable entity IDs and schema version 1.
- JSON serialization/deserialization for the documented model subset.
- Invertible append-only operation-log primitives with undo/redo tests.
- Explicit equal-power pan law (−3 dB center) and selectable −4.5 dB legacy law.
- Float32 mixer primitives, solo-in-place/mute semantics, and deterministic offline render helper.
- Structured error type.
- Offline CMake build and contract tests.

## Not yet complete / do not claim
This is a milestone foundation, not a finished DAW release. ALSA device streaming, lock-free real-time graph swapping with safe reclamation, autosave timer/crash journal, actual bus routing/cycle detection, 60-minute zero-xrun validation, loopback latency measurement, 8-hour soak, and the 500-track performance gate still require implementation and target-machine validation. No GUI is included, as required by Prompt 0.2.

See `Docs/IMPLEMENTATION_STATUS.md`, `Docs/DECISION_LOG.md`, and `Docs/VALIDATION_REPORT.md`.
