# Clip routing audit

## Existing state

- `Project` stores `MediaSource` metadata and clip placement.
- `read_wav()` decodes audio off the callback into interleaved float samples.
- `mix_tracks()` is the existing canonical implementation of track gain, pan law, mute, and solo.
- `render_bus_graph()` accepts pre-built bus inputs and renders bus-to-bus routes; it does not resolve clips or assign tracks to buses.
- The previous clip schema lacked fades and used an unsigned timeline position, which could not express pre-roll.

## This change

- Adds optional schema-v1-compatible `fade_in_frames` and `fade_out_frames`. Older clip JSON without either field reads as zero fades.
- Makes the timeline start signed to represent pre-roll.
- Adds `render_project_clip_tracks()` to sum decoded clips into per-track mono buffers with clip gain and linear fades. Negative-start clips skip source frames before zero; output is truncated to the requested length.
- Adds a hand-computed two-track/four-clip test covering fades, pre-roll, truncation, and canonical track gain/pan.

## Follow-up wiring added

- `Track::bus_id` optionally selects the destination bus and is serialized in schema-v1. Older project JSON without `bus_id` remains readable; an empty value routes to the first configured bus.
- `render_project_clips_to_bus_inputs()` connects decoded clip buffers to per-track sums, then reuses `mix_tracks()` for track gain, pan law, mute, and globally evaluated solo behavior before accumulating into bus direct inputs.
- The new contract test verifies a two-track/two-bus arrangement, sample-exact left/right routing through the bus graph, schema round-trip, missing-bus rejection, and legacy-v1 compatibility.

## Remaining limits

This is an offline/control-thread bridge and allocates. It does not wire clip decoding into the realtime callback, and does not change the realtime graph's capture mapping. The caller must validate the project bus graph and supply decoded sources; real-time clip playback remains a separate gate.
