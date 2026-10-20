# HUSH 0.4a — recording capture foundation

## Delivered slice

RecordingRingBuffer is a preallocated, single-producer/single-consumer interleaved-float ring. The capture side writes complete frames to storage allocated by configure() before audio starts. A non-realtime consumer drains complete frames and can pass them to a disk writer. Ring operations do not allocate, lock, log, or perform file I/O.

If the consumer falls behind, the producer accepts only available whole frames and increments dropped_frames(). Capture never blocks waiting for disk. The application must surface dropped-frame counts and treat any nonzero count as a recording integrity error; it must not silently label the take complete.

## Thread ownership and lifecycle

1. Construct/configure on the control thread while capture and drain are stopped.
2. Start the disk-consumer worker before enabling capture.
3. Only the audio/capture thread calls capture().
4. Only one non-realtime consumer calls drain().
5. Stop/disarm capture first, then let the consumer drain remaining frames and close its file.
6. Do not call configure() concurrently with either side. Reconfiguration is only valid when both sides are stopped.

The class stores interleaved normalized float samples and preserves frame order. Capacity is fixed at configuration; it does not grow while recording.

## Limits and remaining 0.4 work

This 0.4a slice is the bounded capture-buffer primitive and its contract tests. It does not yet provide the worker thread, streaming WAV file finalization, track arming, take UUIDs/playlists, punch modes, comping, crossfades, latency compensation, or actual ALSA capture-to-file wiring. No recording or hardware-validation claim is made until those integrations and the Prompt 4 gates pass.

No runtime network dependency is added. File I/O must remain on the consumer/control side, never in the audio callback.
