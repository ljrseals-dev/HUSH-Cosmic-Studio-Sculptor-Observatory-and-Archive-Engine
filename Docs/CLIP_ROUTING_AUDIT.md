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

## Still open

This does not claim clips are wired into the live callback or into bus nodes. The current project model has bus-to-bus routes but no track-to-bus assignment. A follow-up must define that mapping and connect the clip-rendered buffers to the canonical live/offline mixer and bus graph. This renderer allocates and is for offline/control-thread use only.
