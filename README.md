# HUSH Unified DAW — 0.2 headless foundation

This project follows the ordered headless-first milestone sequence in `Docs/Prompt`. It includes persisted bus routing, a local recovery-journal API, an offline reference renderer, a bounded SPSC parameter queue, a preallocated bus processor, and a realtime interleaved-stereo adapter. GUI, recording, MIDI, plugins, and voice control remain out of scope.

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
- Fixed-capacity SPSC parameter-command queue with explicit full/empty behavior.
- RealtimeBusEngine applies queued fader updates at processing-block boundaries.
- RealtimeBusAudioAdapter maps interleaved stereo capture into a selected graph input bus and emits the selected master's post-fader signal, with preallocated scratch.
- Optional Linux ALSA capture/playback backend, disabled by default.
- Optional `hush_alsa_realtime_host` executable that builds a minimal Capture -> Master graph and owns the graph, engine, adapter, and device lifecycle. It stops on Ctrl-C/SIGTERM.
- Append-only full-snapshot recovery journal with length framing, sync request, and truncated-tail recovery.
- Non-audio-thread autosave controller policy; application lifecycle integration remains required.
- Process-restart integration test for a completed durable journal append; this is not a power-loss durability guarantee.
- Invertible append-only operation-log primitives with undo/redo tests.
- Explicit equal-power pan law (−3 dB center) and selectable −4.5 dB legacy law.
- Float32 mixer primitives, solo-in-place/mute semantics, and deterministic offline render helper.

## Optional ALSA host build and run

On Linux with ALSA development headers/libraries installed:

```sh
cmake -S . -B build-alsa -DHUSH_ENABLE_ALSA=ON -DHUSH_BUILD_TESTS=ON
cmake --build build-alsa
./build-alsa/hush_alsa_realtime_host
```

The host uses the ALSA `default` capture/playback devices at 48 kHz and 128 frames per period. Press Ctrl-C to stop. It is a minimal integration host, not the finished DAW GUI or full track mixer.

## Not yet complete / do not claim
This is not a finished DAW release. The standard CI job tests the ALSA-disabled build; the optional ALSA-enabled host still needs compilation and testing on Linux with ALSA development files and real devices. Safe graph swapping/reclamation, full track-to-bus/insert/solo/mute integration, meters/xrun reporting in the graph, 60-minute zero-xrun validation, loopback latency measurement, 8-hour soak, and the 500-track performance gate require further work and target-machine evidence. No GUI is included.
