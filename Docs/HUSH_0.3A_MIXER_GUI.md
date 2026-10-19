# HUSH 0.3a — Mixer GUI

## Scope and critical limitation

**This GUI edits project state only. It is NOT wired to live audio processing and does not control the running audio engine.** Gain, pan, mute, and solo values are stored in the schema-v1 project and operation log; they are not live DSP commands. Do not describe this as a live mixer.

Optional Qt 6 Widgets shell over the canonical `hush::Project` model. Mixer-only: no arrangement, recording, MIDI, plugin editors, voice control, or live graph reconfiguration.

## Build

Qt 6 Widgets must already be installed; CMake never downloads dependencies.

```sh
cmake -S . -B build-gui -DHUSH_BUILD_TESTS=ON -DHUSH_BUILD_GUI=ON
cmake --build build-gui --parallel 2
ctest --test-dir build-gui --output-on-failure
./build-gui/hush_mixer_gui
```

On Debian/Ubuntu, install `qt6-base-dev` before configuring.

## Implemented

- Open/save schema-v1 JSON projects through the existing reader/writer.
- Track name and stable ID, gain (0.00–2.00), pan (−1.00 to +1.00), mute and solo.
- Mutations use `OperationLog::apply_and_append`; Undo and Redo use the same operation log.
- In-memory demo session on first launch; never automatically saved.
- No telemetry, runtime network requests, update checks, or background audio capture.

## Remaining

- Live engine binding and block-boundary command integration.
- Meter polling, sends/insert UI, track creation/deletion, CLI/GUI parity automation.
- GUI leak/xrun soak tests and hardware validation.

A passing GUI build proves only that this optional target compiles and its registered tests pass. It does not establish live-audio control, hardware latency, zero-xrun behavior, or long-soak stability.
