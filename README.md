# HUSH Unified DAW — 0.2 headless foundation

This project follows the ordered headless-first milestone sequence in `Docs/Prompt`. The current work extends the 0.2 foundation with persisted bus routing, a local recovery-journal API, an offline reference renderer, a bounded SPSC parameter queue, a preallocated bus processor, and an interleaved-stereo adapter for realtime graph processing. GUI, recording, MIDI, plugins, and voice control remain out of scope.

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
- Offline/reference bus-buffer renderer with pre-fader and post-fader sends.
- Preallocated bus processor with control-thread setup, numeric route indices, fixed buffers, and a processing function that does not explicitly allocate, lock, log, or perform file I/O.
- Fixed-capacity SPSC parameter-command queue primitive with full/empty handling and explicit lock-free atomic diagnostic.
- RealtimeBusEngine applies queued fader updates at processing-block boundaries.
- RealtimeBusAudioAdapter maps interleaved stereo capture into a selected graph input bus and copies the selected master's post-fader signal to the output buffer, with preallocated scratch.
- Append-only full-snapshot recovery journal with length framing, sync request, and truncated-tail recovery.
- Non-audio-thread autosave controller policy: first-poll snapshot, periodic snapshots, and save on observed transport stop; host lifecycle integration remains required.
- Process-restart integration test for a completed durable journal append; this is not a power-loss durability guarantee.
- Invertible append-only operation-log primitives with undo/redo tests.
- Explicit equal-power pan law (−3 dB center) and selectable −4.5 dB legacy law.
- Float32 mixer primitives, solo-in-place/mute semantics, and deterministic offline render helper.
- Structured error type and offline CMake contract tests.

## Not yet complete / do not claim
This is a milestone foundation, not a finished DAW release. The adapter is connected to RealtimeBusEngine through a reusable callback; a running application/device lifecycle still needs to construct and own the graph, engine, adapter, and ALSA backend. ALSA device compilation and hardware validation remain pending. Safe graph swapping/reclamation, full track-to-bus/insert/solo/mute integration, meters/xrun reporting in the graph, 60-minute zero-xrun validation, loopback latency measurement, 8-hour soak, and the 500-track performance gate still require implementation and target-machine validation. No GUI is included.

## Optional ALSA backend

Enable with `-DHUSH_ENABLE_ALSA=ON` on Linux with ALSA development files installed. See [`Docs/ALSA_AUDIO_BACKEND.md`](Docs/ALSA_AUDIO_BACKEND.md). Use `RealtimeBusAudioAdapter::alsa_callback` as the ALSA callback and pass the configured adapter as user data.

## Realtime bus engine integration

See [`Docs/REALTIME_BUS_ENGINE.md`](Docs/REALTIME_BUS_ENGINE.md) for the single-producer/single-consumer contract and limitations. The callback adapter does not establish hardware real-time readiness.
