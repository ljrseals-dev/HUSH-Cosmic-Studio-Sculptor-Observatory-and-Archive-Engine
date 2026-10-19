# Optional ALSA audio backend

The Linux-only HUSH_ENABLE_ALSA option builds hush_alsa_backend when ALSA development headers and libraries are installed. It opens separate capture and playback PCM devices in stereo interleaved float32 mode and invokes a supplied callback on a dedicated worker thread. Device names default to `default` and may be configured independently.

## Build

    cmake -S . -B build -DHUSH_ENABLE_ALSA=ON -DHUSH_BUILD_TESTS=ON
    cmake --build build
    ctest --test-dir build --output-on-failure

ALSA is optional and disabled by default. CMake does not install packages or access the network.

## Connect ALSA to the realtime bus engine

1. Configure a valid `BusGraph` on the control thread.
2. Configure `RealtimeBusEngine` with that graph and a maximum period size.
3. Configure `RealtimeBusAudioAdapter` with the engine, the graph bus index that receives capture audio, the graph bus index whose post-fader signal is the master output, and the maximum period size.
4. Start `AlsaAudioBackend` with `RealtimeBusAudioAdapter::alsa_callback` and a pointer to the adapter as user data.

The adapter maps interleaved stereo float input to planar left/right data. It supplies silence to all other bus input slots, calls the engine once per block, then copies the selected master's post-fader output back to interleaved stereo. Graph routing should route the chosen input bus into the chosen master bus. A bus index follows the graph's original `buses()` order. Do not reconfigure or destroy the graph/engine/adapter while the audio worker is running.

## Callback contract

The device callback runs on the backend worker thread. It must not allocate, lock, log, or perform I/O. Adapter buffers are allocated before the worker starts. `stop()` joins the worker before closing PCM handles.

Capture and playback must negotiate the same sample rate and period size; otherwise startup fails rather than silently resampling. `xrun_count()` counts ALSA recoveries and short/zero transfers observed by this implementation. It is diagnostic only, not a substitute for hardware loopback and underrun-injection tests.

## Limitations

- Uses blocking ALSA read/write on a dedicated worker, not ALSA's callback API; callback worst-case execution time still needs measurement.
- Device setup and stop can block and must be called from a control thread.
- This adapter is an explicit graph-input/master-output bridge, not a full track mixer or application host.
- No latency, zero-xrun, hardware-compatibility, or release-readiness claim is made until tested on a named Linux machine.
- On device failure the worker stops; the host should poll `running()`. Detailed worker failure reporting is not exposed yet.
