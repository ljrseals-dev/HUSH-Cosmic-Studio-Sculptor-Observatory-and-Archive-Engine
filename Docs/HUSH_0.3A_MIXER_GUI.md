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
- While live audio is running, gain and pan enqueue preview snapshots as the sliders move, so changes are heard at a subsequent audio block boundary; releasing the slider commits the final project value as one undoable operation. Mute and solo commit immediately. Undo/redo locate the touched operation and enqueue only that track. Startup seeds all tracks before audio begins. This avoids consuming queue capacity proportional to total track count on every edit.
- The bounded queue is single-producer (GUI thread) / single-consumer (audio callback). A rejected/full queue is reported; the GUI does not wait for the audio callback.
- If a committed control snapshot is rejected because the queue is temporarily full, the GUI remembers that track ID and retries the latest committed project state on the UI timer. Each retry is non-blocking, stops after the first full-queue result, and clears when live audio stops. This handles transient saturation without spinning; it cannot guarantee delivery if the audio consumer is not running.
- The callback uses preallocated buffers and performs no allocation, locking, logging, or file/network I/O.
- The backend callback thread is stopped and joined before live mixer state is destroyed or replaced.
- The queue's atomic lock-free properties and actual callback timing must still be checked on target hardware.

## Evidence and remaining gates

The registered `hush_gui_engine_command_path_tests` is a headless deterministic contract test for the same project/OperationLog → bounded track-command queue → next processing block path used by the GUI. It checks gain preview without a project mutation or undo entry, commit, undo, redo, mute, solo, and bounded-queue rejection against synthetic audio buffers. It requires no Qt display, ALSA device, network, or audio hardware.

This is automated proof of the **command/engine contract**, not yet a click-level Qt UI test or physical audio-device session. It deliberately exercises the real `OperationLog` and `RealtimeTrackMixer` implementations, while reproducing the GUI's track-ID lookup and command-submission contract in the test.

Still required before claiming a production live mixer:
- Qt interaction test that clicks/drags the actual widgets and verifies they submit the expected commands;
- a dedicated two-channel loopback integration test;
- full track-source and track-to-bus routing, rather than the current first-two-input-channel mapping;
- target-device startup/shutdown, callback timing, lock-free atomic checks, and a documented soak/latency report. The status counters are diagnostics, not a peak/RMS meter or proof of zero xruns.

Do not claim recording, MIDI, automation, plugin hosting, multitrack live source routing, or zero-xrun hardware behavior as delivered by this change.
