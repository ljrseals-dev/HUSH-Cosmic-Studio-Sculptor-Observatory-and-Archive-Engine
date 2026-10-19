# HUSH 0.3a — Mixer GUI

## Scope
Optional Qt 6 Widgets mixer shell over the canonical schema-v1 hush::Project model. Mixer-only: no arrangement, recording, MIDI, plugin editors, voice control, or live graph reconfiguration.

## Build
Qt 6 Widgets must already be installed; CMake never downloads dependencies.

    cmake -S . -B build-gui -DHUSH_BUILD_TESTS=ON -DHUSH_BUILD_GUI=ON
    cmake --build build-gui --parallel 2
    ./build-gui/hush_mixer_gui

On Debian/Ubuntu, install qt6-base-dev before configuring.

## Implemented
- Open/save schema-v1 JSON projects through the existing reader/writer.
- Track name and stable ID, gain (0.00–2.00), pan (−1.00 to +1.00), mute and solo.
- Mutations use OperationLog::apply_and_append; Undo and Redo use the same operation log.
- In-memory demo session on first launch; never automatically saved.
- No telemetry, runtime network requests, update checks, or background audio capture.

## Remaining
- This is a GUI shell, not a complete DAW.
- Controls currently edit project state only; they are not connected to the live audio graph.
- Meter polling, sends/insert UI, track creation/deletion, CLI/GUI parity automation, and GUI leak/xrun soak tests remain.
- Passing a GUI build does not establish hardware latency, zero-xrun, or long-soak guarantees.
