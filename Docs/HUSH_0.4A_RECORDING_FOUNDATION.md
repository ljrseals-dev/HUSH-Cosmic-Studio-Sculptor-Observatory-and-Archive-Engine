# HUSH 0.4a — Recording foundation

## Scope
This first recording slice adds a preallocated bounded SPSC ring for interleaved float samples, callback-side capture without allocation/locking/logging/I/O, an incremental RIFF/WAVE writer with final-header patching, and a worker-thread session coordinator with preallocated drain scratch and dropped-frame accounting.

## Threading contract
Construct the session before opening the device. Call capture() only from the single audio producer. Call start(), drain(), and stop() only from a non-realtime control/worker thread. drain() writes to disk and may block. Exactly one producer and one consumer are supported. If a whole incoming block will not fit, it is rejected and its frame count is added to dropped_frames; existing samples are not overwritten. The worker must drain frequently enough for the target disk/system.

## Limits
This slice records one interleaved stream (up to 8 channels), not a complete per-track multitrack session. RIFF/WAVE output is limited to less than 4 GiB; RF64/segmented rollover is not implemented. Takes/playlists, active-take persistence, comp overlays, punch, pre/post-roll, crossfades, device-latency measurement/compensation, and GUI controls are not implemented here. No physical device, loopback, 8-track/10-minute, punch-accuracy, or soak test is claimed.

The registered hush_recording_session_tests test covers chunked capture, worker draining, final header, sample order, and overflow reporting. CI must pass before this slice is verified.
