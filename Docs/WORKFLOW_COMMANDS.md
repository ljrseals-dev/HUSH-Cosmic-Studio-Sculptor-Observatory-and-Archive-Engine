# HUSH Workflow Commands and Keyboard Shortcuts

## Contract

Every enabled command in the Qt application must be represented by one stable command ID and one QAction. The same QAction supplies its menu item, shortcut and command-palette entry. Do not implement a menu-only or shortcut-only duplicate action.

A command must:
- Have a stable, namespaced ID.
- Have a user-readable label.
- Be searchable in the command palette.
- Have a keyboard shortcut where a safe default exists.
- Call the same domain/UI handler as its button or menu equivalent.
- Mutate project state through the existing operation/transaction path.
- Reject invalid context without changing project state or undo history.
- Never perform work in the audio callback.

## Current mixer-shell commands

| Command ID | Label | Default shortcut | Context |
|---|---|---|---|
| file.open | Open Project | Ctrl+O / platform standard | Always |
| file.save | Save Project | Ctrl+S / platform standard | Always |
| edit.undo | Undo | Platform standard Undo | When history has an undoable operation |
| edit.redo | Redo | Platform standard Redo | When history has a redoable operation |
| track.mute.toggle | Toggle Mute on Selected Track | M | A track row is selected |
| track.solo.toggle | Toggle Solo on Selected Track | Shift+S | A track row is selected |
| audio.live.toggle | Start / Stop Live ALSA | Ctrl+Alt+L | ALSA-enabled build; explicit user action |
| command.palette | Search Commands | Ctrl+K | Always |

The live-audio action is disabled in builds without ALSA and is not shown among enabled palette results. The command palette itself is omitted from its own result list to prevent recursive invocation.

## Palette behavior

- Ctrl+K opens a local, searchable list of enabled commands.
- Search is case-insensitive and filters the visible command labels/shortcuts.
- Enter or activation invokes the selected command.
- Commands that are disabled in the current build are not advertised as available.
- The palette performs no network access, telemetry, indexing service or background process.

## Expansion rules

When a new feature is added, its commands must be added to the same registry:
- Transport: play, pause, stop, return-to-start, locate, loop.
- Track: add, duplicate, delete/restore, rename, color, folder, arm, input/output.
- Clip: split, trim, move, nudge, duplicate, consolidate, fade, reverse, render.
- Playlist/takes: new playlist, next/previous playlist, solo/unsolo, comp selection.
- Mixer: mute/solo, gain/pan, sends, inserts, VCA, monitor profile, mono/dim.
- Automation: read/touch/latch/write, breakpoint edit, curve shape, copy/paste.
- MIDI: record, note edit, quantize, transpose, CC lane, articulation, MPE expression.
- Delivery: export range, export stems, queue batch, cancel job.
- Session: save snapshot, recall snapshot, switch session, import/transfer.

These future commands are not implemented merely because they appear in this list. Add them only with their corresponding project model, undo, persistence, realtime and test contracts.

## Keymap evolution

The initial implementation provides safe defaults, not a complete shortcut editor. Follow-on work should add:
- User-editable key bindings with per-platform defaults.
- Conflict detection and clear/restore defaults.
- Context scopes (global, timeline, mixer, MIDI editor, text field).
- Import/export of local keymap profiles.
- A local macro/command-sequence runner with bounded execution and explicit undo grouping.
- A discoverability test requiring every enabled QAction to be in the palette and menu registry.

Do not copy proprietary command internals or shortcut tables. Use familiar conventions while keeping HUSH commands independently named and configurable.
