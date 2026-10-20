# HUSH 0.2a implementation plan

This milestone is deliberately based on existing source rather than a parallel engine or a claim that HUSH 0.2 is release-qualified.

## Implemented in 0.2a

- Centralized pan-law calculation in a pure helper used by the mixer.
- Continuous legacy -4.5 dB center pan curve with hard-panned endpoints.
- Defined handling for NaN/Inf pan input (treat as center) and out-of-range control values (clamp).
- Added contract tests for both pan laws, endpoints, center, continuity, invalid controls, and unclamped float32 mixing.
- Registered the test with CTest and kept assertions enabled in Release configurations.
- Recorded source-derived scope and outstanding release gates.

## Acceptance gate

- `hush_pan_law_tests` passes in Debug and Release builds.
- Existing contract tests continue to pass.
- No claim of full 0.2 completion is permitted until the remaining gates in `HUSH_0.2A_SOURCE_RESEARCH.md` have reproducible evidence.

## Deliberate non-goals

No GUI work, recording, MIDI device I/O, plugin hosting, voice control, runtime network access, dependency downloads, or changes to schema-v1 project files.
