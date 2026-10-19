# Optional ALSA audio backend

The Linux-only HUSH_ENABLE_ALSA option builds hush_alsa_backend when ALSA development headers and libraries are installed. It opens separate capture and playback PCM devices in stereo interleaved float32 mode and invokes a supplied callback on a dedicated worker thread. Device names default to `default` and may be configured independently.

## Build

    cmake -S . -B build -DHUSH_ENABLE_ALSA=ON -DHUSH_BUILD_TESTS=ON
    cmake --build build
    ctest --test-dir build --output-on-failure

ALSA is optional and disabled by default. CMake does not install packages or access the network.

## Callback contract

The callback receives capture and playback buffers in interleaved L/R float32 format and the negotiated period length. It runs on the backend worker thread. It must not allocate, lock, log, or perform I/O. Buffers are allocated before the worker starts. stop() joins the worker before closing PCM handles.

Capture and playback must negotiate the same sample rate and period size; otherwise startup fails rather than silently resampling. xrun_count() counts ALSA recoveries and short/zero transfers observed by this implementation. It is diagnostic only, not a substitute for hardware loopback and underrun-injection tests.

## Limitations

- Uses blocking ALSA read/write on a dedicated worker, not ALSA's callback API; callback worst-case execution time still needs measurement.
- Device setup and stop can block and must be called from a control thread.
- Does not yet own/configure RealtimeBusEngine; host callback must adapt buffers to the graph.
- No latency, zero-xrun, hardware-compatibility, or release-readiness claim is made until tested on a named Linux machine.
- On device failure the worker stops; the host should poll running(). Detailed worker failure reporting is not exposed yet.
