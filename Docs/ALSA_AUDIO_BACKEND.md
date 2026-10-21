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

The host configures `RealtimeProjectAudioPipeline` from the loaded project. Persisted bus routes and optional per-track `bus_id` values are applied to live processing; an empty track bus ID routes to the selected input bus. Stereo capture populates source tracks 1 and 2 (left/right); other tracks remain silent until media/device source playback is connected. Track gain, pan, mute, and solo snapshots are applied at block boundaries. Projects without bus arrays use a compatible Capture-to-Master fallback. The host autosaves recovery snapshots on its control loop and reports journal failures. Physical ALSA behavior still requires hardware validation.

## Connect ALSA to the realtime bus engine

The host's project-aware setup sequence is:
1. Load and validate the local project and its optional per-track bus IDs.
2. Configure `RealtimeProjectAudioPipeline` off the audio thread. It resolves track bus IDs against the validated bus graph, preallocates source/output buffers, and configures the bounded track-control queue and published bus engine.
3. Start `AlsaAudioBackend` with `RealtimeProjectAudioPipeline::alsa_callback` and a pointer to the pipeline as user data.
4. Stop/join the backend before reclaiming retired graph snapshots or destroying the pipeline.

The callback maps interleaved stereo capture to the first two mono source tracks, routes all source tracks through their assigned bus inputs, processes the bus graph, and copies master output back to interleaved stereo. Empty per-track bus IDs use the selected input bus. This is a live track-to-bus/master path for available source buffers, not yet a media-playback or recording engine. The lower-level `RealtimeBusEngineHostAudioAdapter` remains available for adapter-specific tests and applications that supply bus inputs directly.

## Explicit stereo capture recording

Pass `--record FILE.wav` to explicitly record the raw stereo ALSA input to a local IEEE-float WAV. The pipeline copies capture frames into a preallocated SPSC ring; a separate `RecordingSession` worker performs disk writes and finalizes the WAV header when recording stops. The CLI allocates five seconds of ring capacity and reports frames written and dropped on shutdown. If the writer fails, the host reports a recording persistence error rather than claiming a complete file.

This records the input capture stream, not the processed master output. It is not multitrack recording: track arming, take/playlist management, punch, comping, crossfades, latency compensation, and zero-dropped-frame device qualification remain open.

## Callback contract

The device callback runs on the backend worker thread. It must not allocate, lock, log, or perform I/O. Adapter buffers are allocated before the worker starts. `stop()` joins the worker before closing PCM handles.

Capture and playback must negotiate the same sample rate and period size; otherwise startup fails rather than silently resampling. `xrun_count()` counts ALSA recoveries and short/zero transfers observed by this implementation. It is diagnostic only, not a substitute for hardware loopback and underrun-injection tests.

## Limitations

- Uses blocking ALSA read/write on a dedicated worker, not ALSA's callback API; callback worst-case execution time still needs measurement.
- Device setup and stop can block and must be called from a control thread.
- Only stereo capture source tracks 1–2 receive samples; other project tracks remain silent until media/device source routing is implemented.
- The adapter's bus count and input-bus index are fixed at configuration; changing topology requires stopping the backend and reconfiguring the adapter.
- No latency, zero-xrun, hardware-compatibility, or release-readiness claim is made until tested on a named Linux machine.
- If capture or playback recovery fails, the backend publishes a lock-free terminal failure category and the raw negative `snd_pcm_recover` status. After the worker has stopped and been joined, the host prints `HUSH_ALSA_WORKER_FAILED side=capture|playback recover_error=<negative-code>` and exits with code 3. Expected errors caused by an intentional `stop()` are not classified as terminal device failures. This identifies the failing stream and preserves the ALSA status code; it does not replace device-specific diagnosis or hardware validation.
