# HUSH 0.3a — Mixer GUI and optional live input monitor

## Scope and limits

The Qt mixer edits the canonical project model and records undoable changes through `OperationLog`. When built with both `HUSH_BUILD_GUI=ON` and `HUSH_ENABLE_ALSA=ON`, an explicitly user-started **Live ALSA** monitor also connects the GUI controls to `RealtimeTrackMixer` through its bounded SPSC command queue.

This is a first live-control integration, not a complete multitrack DAW mixer. The current ALSA backend provides one stereo capture stream: its left channel feeds track index 0 and its right channel feeds track index 1. Additional project tracks have no live source and are disabled while the monitor runs. Track-to-bus routing, independent playback sources, inserts/sends, recording, MIDI, plugin hosting, and external sync are not implemented by this change.

No audio capture begins automatically. The user must press **Start Live ALSA**; stopping the monitor joins the backend thread before its mixer state is released. There are no telemetry, network requests, update checks, or background uploads.

## Build

Dependencies must already be installed; CMake does not download them.

GUI without live ALSA:

```sh
cmake -S . -B build-gui -DHUSH_BUILD_TESTS=ON -DHUSH_BUILD_GUI=ON
cmake --build build-gui --parallel 2
ctest --test-dir build-gui --output-on-failure
./build-gui/hush_mixer_gui
```

GUI with optional Linux ALSA live monitor (Qt 6 Widgets and ALSA development headers required):

```sh
cmake -S . -B build-gui-alsa -DHUSH_BUILD_TESTS=ON -DHUSH_BUILD_GUI=ON -DHUSH_ENABLE_ALSA=ON
cmake --build build-gui-alsa --parallel 2
ctest --test-dir build-gui-alsa --output-on-failure
./build-gui-alsa/hush_mixer_gui
```

The default capture and playback devices are ALSA `default`, at a requested 48 kHz and 128 frames per period. Actual values depend on the target device. If the device cannot open/configure both capture and playback, the GUI reports a startup error. Device compatibility, loopback, latency, xrun rate, and long-duration stability require target-machine tests.

## Realtime contract

- `RealtimeTrackMixer` is configured and its track buffers are allocated before starting the backend.
- GUI gain, pan, mute, and solo edits enqueue a complete control snapshot for the affected track; undo/redo locate the touched operation and enqueue only that track. Startup still seeds all tracks before audio begins. This avoids consuming queue capacity proportional to total track count on every edit.
- The bounded queue is single-producer (GUI thread) / single-consumer (audio callback). Edits, undo, and redo reserve their live command before committing project state/history; if the queue is full, the operation is rejected, the model/history do not advance, the UI is restored from canonical state, and a retry message is shown. The GUI never waits for the audio callback.
- `RealtimeTrackMixer` rejects unknown track indices, unconfigured use, non-finite gain/pan, gain outside 0–2, and pan outside −1–1. Queue saturation returns `false`; it does not overwrite pending commands.
- The callback uses preallocated buffers and performs no allocation, locking, logging, or file/network I/O.
- The backend callback thread is stopped and joined before live mixer state is destroyed or replaced.
- The queue's atomic lock-free properties and actual callback timing must still be checked on target hardware.

## Evidence and remaining gates

The repository's CI should compile the combined Qt + ALSA target and run the registered core contract tests. That verifies build compatibility and existing mixer queue contracts, not a physical audio-device session.

Still required before claiming a production live mixer:
- automated Qt interaction test proving fader/pan/mute/solo and undo/redo are reflected in the next audio block; current core regression tests prove command application and deterministic queue-full/invalid-value rejection, but do not synthesize Qt clicks;
- a dedicated two-channel loopback integration test;
- full track-source and track-to-bus routing, rather than the current first-two-input-channel mapping;
- target-device startup/shutdown, callback timing, lock-free atomic checks, and a documented soak/latency report. The status counters are diagnostics, not a peak/RMS meter or proof of zero xruns.

Do not claim recording, MIDI, automation, plugin hosting, multitrack live source routing, or zero-xrun hardware behavior as delivered by this change.
