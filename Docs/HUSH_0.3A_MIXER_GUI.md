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
- While live audio is running, gain and pan enqueue preview snapshots as the sliders move, so the next audio block uses the changing values. Releasing the slider commits the final value as one undoable project operation; preview movement itself does not mutate the project or create undo entries. Mute and solo commit immediately. Undo/redo locate the touched operation and enqueue only that track. Startup seeds all tracks before audio begins.
- The bounded queue is single-producer (GUI thread) / single-consumer (audio callback). Edits, undo, and redo reserve their live command before committing project state/history; if the queue is full, the operation is rejected, the model/history do not advance, the UI is restored from canonical state, and a retry message is shown. The GUI never waits for the audio callback.
- `RealtimeTrackMixer` rejects unknown track indices, unconfigured use, non-finite gain/pan, gain outside 0–2, and pan outside −1–1. Queue saturation returns `false`; it does not overwrite pending commands.
- The callback uses preallocated buffers and performs no allocation, locking, logging, or file/network I/O.
- The backend callback thread is stopped and joined before live mixer state is destroyed or replaced.
- The queue's atomic lock-free properties and actual callback timing must still be checked on target hardware.

## Evidence and remaining gates

The registered `hush_gui_engine_command_path_tests` is a deterministic, headless synthetic-buffer contract test for the project/OperationLog → bounded track-command queue → next processing block path used by the mixer GUI. It checks gain preview without project mutation or an undo entry, commit, undo, redo, mute, solo, and bounded-queue rejection using the production `OperationLog` and `RealtimeTrackMixer` implementations. It requires no Qt display, ALSA device, network, or physical audio hardware.

This verifies the command/engine contract at the core API level, not actual Qt widget signal delivery. The test mirrors the GUI's stable-track-ID lookup and command submission; it is not a click-level Qt test and does not establish hardware loopback or audible device behavior.

Still required before claiming a production live mixer:
- Qt interaction test that clicks/drags the actual widgets and verifies they submit the expected commands;
- a dedicated two-channel loopback integration test;
- full track-source and track-to-bus routing, rather than the current first-two-input-channel mapping;
- target-device startup/shutdown, callback timing, lock-free atomic checks, and a documented soak/latency report. The status counters are diagnostics, not a peak/RMS meter or proof of zero xruns.

Do not claim recording, MIDI, automation, plugin hosting, multitrack live source routing, or zero-xrun hardware behavior as delivered by this change.
