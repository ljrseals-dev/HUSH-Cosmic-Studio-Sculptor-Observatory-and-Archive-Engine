# HUSH 0.4a — Recording foundation

## Implemented source slice
- Preallocated SPSC interleaved-float capture ring with explicit dropped-frame accounting.
- Incremental IEEE float32 RIFF/WAVE writer that patches file sizes on finalization.
- RecordingSession with preallocated worker scratch, worker-owned disk I/O, and a callback-facing capture method.
- Explicit local path only; no network, telemetry, or hidden/background capture.

## Realtime contract
The capture callback must not allocate, lock, log, or perform file I/O. There is one producer and one consumer. Configure only while stopped. The writer is worker-thread-only. Standard RIFF/WAVE has a 4 GiB limit; RF64 is not implemented.

## Still not complete
This is a recording foundation, not all of Prompt 4. ALSA capture callback integration, persisted take UUIDs/playlists/active-take state, manual/auto/loop punch and pre/post-roll, comp overlays, crossfades, device latency measurement/compensation, and the 8-track by 10-minute acceptance run remain open. Do not claim complete multitrack recording or hardware readiness.

## Validation
Build and run CTest on the branch. Hardware validation must record device/interface, OS/kernel, sample rate, period size, duration, frame counts, dropped frames, and playback comparison.