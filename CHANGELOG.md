# Changelog

Notable repository changes are recorded here. This is a development changelog, not a claim of a finished DAW release.

## Unreleased

- Added optional Qt 6 0.3a mixer shell with schema-v1 project open/save, gain/pan/mute/solo controls, and undo/redo.
- Added CI coverage for standard C++20, ALSA-enabled compilation/contracts, and Qt 6 GUI compilation/contracts.
- Explicitly retain live-engine binding, safe graph reclamation, hardware audio validation, soak, and performance gates as incomplete.
- Clarify journal claims: process-restart recovery is tested; power-loss durability is not established.
