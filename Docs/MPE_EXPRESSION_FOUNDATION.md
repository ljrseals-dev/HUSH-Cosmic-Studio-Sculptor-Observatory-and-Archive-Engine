# MPE Per-Note Expression Foundation

## Why this slice exists

Pro Tools 2026.11 makes MPE a concrete DAW workflow: each note in a chord can carry independent pitch, pressure, and timbre expression, with note-channel color identity. HUSH already has MIDI CC-to-parameter learning, but that is not MPE: a channel-wide controller map cannot represent expression attached to one note.

## Implemented in this slice

- A note-scoped MPE model with stable note ID, zero-based channel, MIDI key/velocity, integer start frame, and duration.
- Independent pitch-semitone, pressure, and timbre expression points for each note.
- Deterministic expression-point ordering and duplicate rejection.
- Validation for channel/key/velocity bounds, note duration and timeline overflow, expression point range, finite values, and point ordering.
- Allocation-free linear evaluation of one note's expression curve, with endpoint hold behavior.
- Stable MIDI-channel color index for UI palette mapping.
- CMake build and contract test registration.

The expression evaluator performs no allocation and is suitable for a prevalidated immutable note snapshot. Validation and project editing remain control-thread work.

## Explicitly not complete

This is a core data/evaluation foundation, not a finished MPE editor. It does not yet provide:
- project JSON persistence, schema migration, transaction/undo integration, or recovery-journal coverage;
- MIDI device enumeration, MPE zone configuration, live MIDI input/output, or note scheduling;
- piano-roll/MIDI-editor lanes, per-note drawing/selection, channel color rendering, or command-palette actions;
- instrument parameter delivery or end-to-end render playback.

Do not advertise MPE as user-complete until the model is persisted and undoable, the MIDI input scheduler attaches expression to stable note IDs, the editor exposes per-note lanes, and playback tests prove independent simultaneous-note output.

## Acceptance sequence

1. Persist notes and expression arrays with strict JSON validation and backward-compatible migration.
2. Add structural transaction operations for note create/delete/move/resize and expression point add/move/delete; undo/redo must restore exact values.
3. Add MPE zone/device input mapping and bounded MIDI event ingestion outside the audio callback.
4. Add per-note expression lanes and channel color coding to the MIDI editor.
5. Deliver expression to the instrument voice allocator at block boundaries without callback allocation, locks, or I/O.
6. Test overlapping notes on different channels with independent pitch/pressure/timbre curves, malformed sessions, recovery, undo/redo, offline render, ASan, and TSan.

## Reference

Avid's Pro Tools 2026.11 release notes document native MPE record/playback/edit/automation, independent per-note pitch/timbre/amplitude, dedicated expression lanes, and MIDI-channel color coding. HUSH's model uses independent note-scoped data and does not copy proprietary implementation details.
