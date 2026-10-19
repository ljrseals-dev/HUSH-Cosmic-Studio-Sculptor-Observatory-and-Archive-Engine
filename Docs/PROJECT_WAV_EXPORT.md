# Offline project bus-to-WAV export

`render_project_bus_to_wav` connects the schema-v1 `Project` bus definitions/routes, the existing offline `render_bus_graph`, and the WAV writer.

## Contract

- Rebuilds and validates a `BusGraph` from the project's canonical bus and route arrays.
- Requires the caller to select an explicit output bus; no master bus is guessed from a name.
- Requires one decoded/prepared stereo direct-input buffer per configured bus, using the existing `BusAudioInput` contract.
- Writes the selected bus' post-fader output as stereo PCM16 or IEEE float32 WAV.
- Reports structured project/render errors and propagates file-writer failures.

This function allocates buffers and writes files. Call it only on an offline/control thread, never from the audio callback.

## Scope boundary

The schema-v1 `Project` currently contains track control metadata, buses, and routes, but no clip timeline or media-source references. The adjacent `read_wav` API now imports RIFF/WAVE PCM16 and IEEE float32 into an interleaved float buffer on a non-realtime thread. It preserves the file's sample rate and channel count; resampling, channel mapping, media-source persistence, clip placement, and complete arrangement rendering are not included yet.
