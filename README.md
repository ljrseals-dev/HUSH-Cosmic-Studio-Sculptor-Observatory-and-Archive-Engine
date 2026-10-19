# HUSH Unified DAW — 0.2 headless foundation

This project follows the ordered milestone sequence in `Docs/Prompt`. It includes persisted bus routing, a local recovery-journal API, an offline reference renderer, a bounded SPSC parameter queue, a preallocated bus processor, a realtime interleaved-stereo adapter, and an optional first-pass 0.3a mixer GUI. Recording, MIDI, plugin hosting, and voice control remain out of scope.

## Build offline

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DHUSH_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

No FetchContent, package bootstrap, network access, telemetry, or update checks are used by CMake or the runtime engine.

## Included

- C++20 project model with stable entity IDs and schema version 1.
- JSON serialization/deserialization for tracks, buses, and routes; older schema-v1 projects without bus/route arrays remain readable.
- Bus-route validation at project load, including cycle detection.
- Offline/reference bus-buffer renderer with pre-fader and post-fader sends.
- Preallocated bus processor with control-thread setup, numeric route indices, fixed buffers, and a processing function that does not explicitly allocate, lock, log, or perform file I/O.
- Fixed-capacity SPSC parameter-command queue with explicit full/empty behavior.
- RealtimeBusEngine applies queued fader updates at processing-block boundaries.
- RealtimeBusAudioAdapter maps interleaved stereo capture into a selected graph input bus and emits the selected master's post-fader signal, with preallocated scratch.
- Optional Linux ALSA capture/playback backend and minimal host, disabled by default.
- Append-only full-snapshot recovery journal with length framing, sync request, and truncated-tail recovery.
- Non-audio-thread autosave controller policy; application lifecycle integration remains required.
- Process-restart integration test for recovering a completed journal append.
- Invertible append-only operation-log primitives with undo/redo tests.
- Optional Qt 6 mixer shell with schema-v1 project open/save and gain/pan/mute/solo/undo/redo controls.

## Optional ALSA host

On Linux with ALSA development headers/libraries installed:

```sh
cmake -S . -B build-alsa -DHUSH_ENABLE_ALSA=ON -DHUSH_BUILD_TESTS=ON
cmake --build build-alsa
./build-alsa/hush_alsa_realtime_host
```

The host uses the ALSA `default` capture/playback devices at 48 kHz and 128 frames per period. Press Ctrl-C to stop. It is a minimal integration host, not the finished DAW GUI or full track mixer.

## Optional mixer GUI (0.3a)

Install Qt 6 Widgets from your system package manager, then build the optional desktop mixer:

```sh
cmake -S . -B build-gui -DHUSH_BUILD_TESTS=ON -DHUSH_BUILD_GUI=ON
cmake --build build-gui --parallel 2
./build-gui/hush_mixer_gui
```

See [Docs/HUSH_0.3A_MIXER_GUI.md](Docs/HUSH_0.3A_MIXER_GUI.md) for scope and limitations. The GUI currently edits project state only; it is not connected to live audio processing.

## Release gates and limitations

This is not a finished DAW release. The standard, ALSA-enabled, and Qt 6 CI jobs compile and run the current contract tests. ASan/TSan CI jobs are being added, but their results must be green before they count as evidence.

The following remain open: GUI-to-live-engine command binding and GUI interaction tests; safe graph swap/reclamation; complete track-to-bus/master routing with inserts and mute/solo; graph meters and xrun readouts; real-device startup/shutdown; callback timing; 60-minute zero-xrun validation; loopback latency and underrun injection; 500-track performance; and 8-hour soak with memory-drift reporting. See [Docs/RELEASE_GATES.md](Docs/RELEASE_GATES.md) and [ROADMAP.md](ROADMAP.md).

**Durability scope:** recovery of completed journal records after process restart is tested, and truncated-tail recovery is covered. HUSH does **not** currently claim power-loss durability. Do not infer target-hardware real-time readiness from CI compile/test success.
