# Channel Light prototype

Channel Light is the first integrated status-and-meter panel in the existing Qt 6 mixer.

## Implemented in this milestone

- The optional ALSA live mixer measures input and output peak amplitude in the audio callback.
- Peak values are published as atomic floats; the Qt GUI reads them on its existing 250 ms timer. The callback never calls Qt or updates widgets.
- Input, processing, clipping, and output states are shown as explicit labels and status lights.
- Peak meters display measured block peaks, clamped to the display range.
- Existing gain, pan, mute, and solo controls remain wired to `RealtimeTrackMixer` via the bounded SPSC parameter queue.
- Dark studio styling uses cyan for signal, violet for processing, and green for healthy/clear state. Clipping is red.
- No telemetry, network requests, or remote services are added.

## Build

```sh
cmake -S . -B build -DHUSH_BUILD_GUI=ON -DHUSH_ENABLE_ALSA=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Qt 6 Widgets and ALSA development headers/libraries must be installed for this optional GUI/backend configuration. To build the GUI without ALSA, use `-DHUSH_BUILD_GUI=ON`; live input stays disabled and meters remain idle.

## Honest limits

This panel measures callback-block peaks, not true-peak oversampled levels or per-channel pre/post-effect levels. The current live backend routes stereo capture to the first two mono tracks; other tracks do not yet have live sources. The processing indicator reports that the live callback is running, not that an effect rack is active. No build or runtime success is claimed until CI or a local build confirms it.
