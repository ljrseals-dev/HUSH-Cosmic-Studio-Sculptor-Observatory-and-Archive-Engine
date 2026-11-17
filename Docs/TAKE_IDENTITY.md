# Persisted Take Identity

## Implemented model contract

Audio clips now carry optional `take_id` and `take_number` fields in the existing schema-v1 project model. Both fields serialize with the clip and round-trip through project loading. Older schema-v1 files remain readable; absent take fields map to the legacy default (empty ID, number zero).

Clips recorded simultaneously on separate tracks can share one take ID and take number. The loader rejects a project in which clips with the same take ID disagree about the take number. `take_color_index(take_id)` derives a deterministic palette slot from the stable ID, so every clip in the same take group receives the same color without persisting a theme-specific RGB value.

## Remaining end-to-end work

This is persisted identity infrastructure, not a completed take/comping workflow. The recording coordinator must still create one take ID per simultaneous recording pass and attach it to every captured track clip. Playlist operations must preserve identity when moving, splitting, duplicating, or comping clips, with explicit semantics for a newly recorded pass versus a duplicate. A take/playlist editor and undoable commands are not yet implemented.

Acceptance requires a multitrack recording test that records several tracks in one pass, verifies identical take IDs and colors, saves/reopens the project, performs playlist edits, and proves identity remains consistent through undo/redo and recovery.
