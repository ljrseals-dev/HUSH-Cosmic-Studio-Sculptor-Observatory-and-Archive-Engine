# Recording Take Coordinator

`assign_recording_take` assigns a new take identity to a validated set of staged clips. Supply every clip created by one simultaneous recording pass, including clips on different tracks. It assigns one project-scoped ID and monotonically increasing take number to the set, giving all clips a consistent `take_color_index`.

The operation is transactional: it validates all requested clips and rejects duplicates, missing clips, pre-identified clips, empty project IDs, and take-number exhaustion before changing any clip. It is a control-thread/project-model operation and must never run in the audio callback.

This is a coordinator primitive, not automatic wiring from live capture or a playlist/comping UI. The recording controller must call it after creating all clips for a pass and before committing the project edit to undo/recovery history.