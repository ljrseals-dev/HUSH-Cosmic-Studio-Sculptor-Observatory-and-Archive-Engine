# Project media sources and sample-positioned clips

Schema-v1 projects now accept two optional top-level arrays:

- `media_sources`: stable source ID, project-relative asset path, source sample rate, channel count, and source frame count.
- `clips`: stable clip ID, track/source references, timeline start frame, source in-point frame, frame count, and clip gain.

All positions are integer frames. No seconds-to-frames conversion is performed by persistence, so placement is sample-positioned without floating-point drift. A clip must reference an existing track and media source, have a unique ID, a finite non-negative gain, a positive frame count, and a source range wholly within the referenced media source. Timeline-end overflow is rejected.

The arrays are additive to schema version 1 and optional when reading older schema-v1 projects. Existing projects without them continue to load. This does not change the schema version or reinterpret existing track fields.

## Current boundary

This commit persists and validates arrangement metadata only. It does not yet decode source files automatically when opening a project, resample sources, build track audio buffers, apply track gain/pan/mute/solo to clip playback, or render the arrangement into the bus graph. Those are subsequent integration steps. The WAV reader remains an offline/control-thread API; file I/O and allocation must not occur in the realtime callback.
