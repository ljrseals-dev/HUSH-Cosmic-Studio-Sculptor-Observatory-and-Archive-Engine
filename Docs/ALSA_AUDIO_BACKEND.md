# Optional ALSA audio backend

The Linux-only HUSH_ENABLE_ALSA option builds `hush_alsa_backend` and the `hush_alsa_realtime_host` executable when ALSA development headers and libraries are installed. The backend opens separate capture and playback PCM devices in stereo interleaved float32 mode and invokes a supplied callback on a dedicated worker thread. Device names default to `default`.

## Build and run

    cmake -S . -B build-alsa -DHUSH_ENABLE_ALSA=ON -DHUSH_BUILD_TESTS=ON
    cmake --build build-alsa
    ctest --test-dir build-alsa --output-on-failure
    ./build-alsa/hush_alsa_realtime_host

ALSA is optional and disabled by default. CMake does not install packages or access the network. The sample host uses 48 kHz and 128 frames per period and stops on Ctrl-C or SIGTERM.

## Optional local project recovery

The host accepts `--help`, `--project FILE`, and `--journal FILE`. When a project is supplied, it validates the local schema-v1 JSON before starting audio. The journal defaults to `FILE.hush-recovery.journal`. If the journal exists but cannot be opened or contains no complete valid snapshot, the host reports `HUSH_RECOVERY_FAILED` and exits with code 2 rather than silently continuing. When a newer recovery snapshot differs from the project file, the host writes the recovered project file and appends the canonical snapshot before starting ALSA.

This is recovery/session metadata handling only: the current host still routes synthetic Capture-to-Master audio and does not apply the loaded project's buses/tracks to live routing. It has no project-edit command surface, so normal operation does not dirty the loaded project or perform a final save. The recovery error path is tested before device startup using a deliberately invalid journal; physical ALSA behavior still requires hardware validation.

## Connect ALSA to the realtime bus engine

The host demonstrates the required control-thread setup sequence:
1. Configure a valid `BusGraph` with a Capture bus routed to a Master bus.
2. Configure `RealtimeBusEngineHost` with the validated graph, master bus ID, and maximum period size. The host builds the replacement snapshot off the callback and publishes it safely.
3. Configure `RealtimeBusEngineHostAudioAdapter` with the host, input bus index, bus count, and maximum period size.
4. Start `AlsaAudioBackend` with `RealtimeBusEngineHostAudioAdapter::alsa_callback` and a pointer to the adapter as user data.

The adapter maps interleaved stereo float input to planar left/right data. It supplies silence to all other bus input slots, calls the published host once per block, then copies the selected master's post-fader output back to interleaved stereo while the host's read guard is held. A bus index follows the graph's original `buses()` order. Same-topology graph replacements may be published while the worker runs. Stop/join the worker before changing bus count/order, reconfiguring the adapter, or destroying the host.

## Callback contract

The device callback runs on the backend worker thread. It must not allocate, lock, log, or perform I/O. Adapter buffers are allocated before the worker starts. `stop()` joins the worker before closing PCM handles.

Capture and playback must negotiate the same sample rate and period size; otherwise startup fails rather than silently resampling. `xrun_count()` counts ALSA recoveries and short/zero transfers observed by this implementation. It is diagnostic only, not a substitute for hardware loopback and underrun-injection tests.

## Limitations

- Uses blocking ALSA read/write on a dedicated worker, not ALSA's callback API; callback worst-case execution time still needs measurement.
- Device setup and stop can block and must be called from a control thread.
- The host is a minimal Capture -> Master integration harness using guarded graph replacement, not a full track mixer or application GUI.
- The adapter's bus count and input-bus index are fixed at configuration; changing topology requires stopping the backend and reconfiguring the adapter.
- No latency, zero-xrun, hardware-compatibility, or release-readiness claim is made until tested on a named Linux machine.
- On device failure the worker stops; the host prints final callback and recovery counters when control returns. Detailed worker failure reporting is not exposed yet.
