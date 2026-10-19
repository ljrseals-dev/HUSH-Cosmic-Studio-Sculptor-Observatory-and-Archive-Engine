# HUSH 0.3a — Mixer GUI

## Scope and critical limitation

**This GUI still edits project state only. It is NOT wired to live audio processing and does not control the running audio engine.** A separate `RealtimeTrackMixer` core primitive has now been added with a bounded SPSC command queue and block-boundary gain/pan/mute/solo updates, but the Qt widgets do not yet enqueue those commands or attach to a running device callback. Do not describe the GUI as a live mixer yet.

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

## Realtime engine primitive (added; not yet GUI-integrated)

- `hush::RealtimeTrackMixer` is configured once on the control thread with a fixed track count and maximum block size.
- The control thread enqueues complete gain/pan/mute/solo snapshots through a bounded SPSC queue; the audio callback applies queued snapshots at the next block boundary.
- The processing method uses preallocated state and performs no allocation, lock, logging, or I/O. One producer and one audio consumer are required.
- A core contract test is registered as `hush_realtime_track_mixer_tests`; it has not been executed in this session.

## Remaining

- Bind Qt control changes (including Undo/Redo) to a configured `RealtimeTrackMixer` and a running audio callback; add a Qt interaction/integration test.
- Meter polling, sends/insert UI, track creation/deletion, CLI/GUI parity automation.
- GUI leak/xrun soak tests and hardware validation.

A passing GUI build proves only that this optional target compiles and its registered tests pass. It does not establish live-audio control, hardware latency, zero-xrun behavior, or long-soak stability.
