# HUSH Unified DAW — 0.2 headless foundation

This project follows the ordered headless-first milestone sequence in `Docs/Prompt`. This milestone extends the 0.2 foundation with persisted bus routing and a local recovery-journal API. GUI, recording, MIDI, plugins, and voice control remain out of scope.

## Build offline

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DHUSH_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

No FetchContent, package bootstrap, network access, telemetry, or update checks are used by this build.

## Included
- C++20 project model with stable entity IDs and schema version 1.
- JSON serialization/deserialization for tracks, buses, and routes; older schema-v1 projects without bus/route arrays remain readable.
- Bus-route validation at project load, including cycle detection.
- Append-only full-snapshot recovery journal with length framing, sync request, and truncated-tail recovery.
- Non-audio-thread autosave controller policy: first-poll snapshot, periodic snapshots, and save on observed transport stop; host lifecycle integration remains required.
- Process-restart integration test for a completed durable journal append; this is not a power-loss durability guarantee.
- Invertible append-only operation-log primitives with undo/redo tests.
- Explicit equal-power pan law (−3 dB center) and selectable −4.5 dB legacy law.
- Float32 mixer primitives, solo-in-place/mute semantics, and deterministic offline render helper.
- Structured error type and offline CMake contract tests.

## Not yet complete / do not claim
This is a milestone foundation, not a finished DAW release. Autosave scheduling is not yet connected to an application lifecycle. ALSA device streaming, lock-free real-time graph swapping with safe reclamation, bus-buffer rendering/send processing, real meters/xrun reporting, 60-minute zero-xrun validation, loopback latency measurement, 8-hour soak, and the 500-track performance gate still require implementation and target-machine validation. No GUI is included.

See `Docs/IMPLEMENTATION_STATUS.md`, `Docs/DECISION_LOG.md`, `Docs/VALIDATION_REPORT.md`, and `Docs/RECOVERY_JOURNAL.md`.
