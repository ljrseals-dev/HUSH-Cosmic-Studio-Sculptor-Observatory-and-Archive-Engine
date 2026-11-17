# Completed-DAW Reference and HUSH Integration Map

Date: 2026-11-17

## Purpose

Use a mature, working DAW as a workflow and architecture reference while keeping HUSH's implementation independent. This document maps complete user workflows to HUSH source boundaries and defines the integration transitions required before a feature is counted as implemented.

## Reference selection

### Busy Works Beats — Pro Tools user workflows

- [Pro Tools Mixing Tutorial](https://www.youtube.com/watch?v=YAbfcDnYM0E) — Busy Works Beats, 2019-12-03.
- [Making Beats in Pro Tools Just Got WAY Easier — Pro Tools Sketch](https://www.youtube.com/watch?v=64x4SsVx3io) — Busy Works Beats, 2023-09-22.
- [Mixing and Mastering FULL tutorial](https://www.youtube.com/watch?v=LtAgD2u_UYg) — Busy Works Beats, 2024-02-20.

Use these as demonstrations of user-facing production tasks (create/import, arrange, edit, mix, automate, render), not as authoritative statements about every Pro Tools feature or shortcut. Shortcut behavior must be checked against the current Avid shortcut guide and the relevant platform/version.

## Mature open-source implementation reference

- [Ardour source repository](https://github.com/Ardour/ardour) — mature C++ DAW source.
- [Ardour manual](https://manual.ardour.org/ardourmanual.html)
- [Ardour feature overview](https://ardour.org/features.html)

The repository was inspected at its current `master` tree. Relevant subsystem boundaries include:

| DAW subsystem | Ardour reference source | HUSH destination / boundary | Required transition |
|---|---|---|---|
| Session/project ownership | `libs/ardour/ardour/session.h`, `libs/ardour/session.cc` | `include/hush/project.hpp`, `src/project.cpp` | Project model ↔ versioned persistence ↔ recovery |
| Track/recording lifecycle | `libs/ardour/ardour/track.h` | Track model, recording session, ALSA host | Device input → armed track → durable media/take → playlist |
| Regions/playlists/comping | `libs/ardour/ardour/playlist.h`, audio/MIDI playlist sources | Project clips and future playlist/take model | Edit command → region/playlist transaction → undo → save/reopen |
| Mixer/routing/processors | `libs/ardour/ardour/route.h` | `bus_graph`, project runtime graph and mixer | Persisted topology → validated graph → bounded realtime publication |
| Automation | `libs/ardour/ardour/automation_list.h` | Current AutomationLane and future persisted automation entities | Parameter identity → time-domain points → playback/write modes → persistence |
| MIDI | `libs/ardour/ardour/midi_track.h` | MIDI event/track subsystem (not yet complete in HUSH main) | Device/SMF input → event track → scheduler → piano-roll editing → save/export |
| Export | `libs/ardour/ardour/export_handler.h` | HUSH renderer and future export queue | Range/stem config → deterministic render → report/cancel/retry |
| Control surfaces | `libs/ctrl-interface/control_protocol/control_protocol/control_protocol.h` | HUSH command/action registry and future control-surface adapter | Hardware/protocol event → stable command ID → domain transaction → feedback |

Ardour's own manual documents multitrack audio/MIDI recording, nondestructive editing, sample-accurate automation, plugin processing, external control surfaces, MIDI Machine Control, MIDI timecode/LTC and flexible signal routing. These are useful parity checkpoints, not a mandate to clone Ardour's internal design.

## Licensing and code reuse boundary

Ardour's source files carry GPL-2.0-or-later headers and the repository's `COPYING` file contains GPLv2. Do not copy Ardour implementation code into HUSH until the HUSH repository's intended distribution license and compatibility have been explicitly reviewed. This work uses Ardour as a source-reading and architecture reference only; HUSH implementations should be independently authored, with compatible public protocol/API specifications used where appropriate. A license audit is required before any source-level port.

## HUSH current-main findings

Baseline reviewed: `main` at `8e4b16af1e4fd9bd18f7b2c1c55e664b8dbfaa43`.

1. Track records do not persist a bus ID. The bus graph cannot fully reconstruct track-to-bus routing.
2. The live Qt ALSA monitor currently maps capture L/R to the first two tracks; it is not a general multi-input recorder/player.
3. OperationLog covers only gain, pan, mute and solo; it is not a general project transaction/history system.
4. AutomationLane is an evaluator, not a persisted automation entity connected end-to-end to plugin/track/bus parameters.
5. MidiLearnMap is a CC-to-numeric-parameter mapping, not a MIDI track/event scheduler, piano roll or MPE system.
6. The command palette implementation is on PR #99, not main.
7. Project-aware mixer, recording/autosave and live meter work is spread across stale/overlapping PRs #94, #95, #74, #81 and #84. Do not count it as main-branch capability.

## Implementation slices — vertical, testable, no disconnected feature flags

### Slice A — Canonical project routing

- Reconcile the track `bus_id` model and serializer with the runtime graph.
- Validate unknown IDs, missing master/input buses, cycles, duplicate IDs and unsupported route topologies before publication.
- Use one project graph for offline render, ALSA live processing and Qt mixer.
- Prepare graph replacements on the control thread; publish at block boundaries; reclaim retired graphs off the audio callback.
- Add JSON round-trip, old-project migration, invalid-graph, deterministic-render and live-output tests.
- Undo and redo route changes as project transactions.

### Slice B — Recording and take lifecycle

- Add explicit input/device/channel mapping and track record-arm state.
- Capture to a bounded ring; write files only on a worker thread.
- Create durable source/take metadata and playlist entries only after successful finalization.
- On failure, retain the previous valid project and surface a structured error.
- Add loop/punch, multiple takes, comp ranges and crossfade transitions after the base capture path is stable.

### Slice C — Editing and project history

- Introduce stable IDs for clips, playlists, takes, routes and automation lanes.
- Model edits as validated transactions with before/after state.
- Implement undo/redo for create, delete, split, trim, move, gain, fade, route and insert.
- Persist history or a recoverable transaction journal with explicit crash semantics.
- Keep view selection/zoom separate from canonical project state.

### Slice D — Automation and MIDI

- Persist automation lane identity, target parameter, time domain, interpolation and points.
- Add read/touch/latch/write semantics and sample-accurate playback delivery.
- Add MIDI track, event, note, CC, pitch-bend, pressure and per-note expression models.
- Add SMF import/export, deterministic event scheduling, piano roll and MPE only after the event model is complete.

### Slice E — Commands and protocols

- Route every menu, shortcut, palette, script and control-surface event through the same stable command ID.
- Command handler validates and creates a project transaction; it must not mutate the audio graph directly.
- Provide editable keymaps, conflict detection, import/export and context-aware commands.
- Add documented OSC/MIDI control-surface protocols only after command identity, feedback and transaction semantics are stable.

### Slice F — Plugin host and delivery

- Define plugin format, scanner, process isolation, watchdog, state serialization, latency reporting and PDC.
- Keep plugin instantiation and graph rebuild off the audio callback.
- Add export queue, range/marker/stem selection, naming templates, cancellation and deterministic render reports.
- Gate AAF/OMF and video/timecode behind explicit format and validation milestones.

## Acceptance rule: close every transition

A feature is complete only when evidence demonstrates:

1. User intent is discoverable through menu, palette, shortcut or documented protocol.
2. The command validates and creates a single domain transaction.
3. Undo/redo restores exact before/after state.
4. Save/reopen and schema migration preserve the state.
5. The runtime receives a bounded, prepared update.
6. UI meters/status observe it without calling the GUI from the audio thread.
7. Restart/recovery leaves a valid durable project.
8. Offline render matches the expected deterministic result.
9. Invalid operations fail without partial mutation.
10. CI evidence identifies the exact commit and test configuration.

## Next integration order

1. Review/merge PR #98 persistence and recovery hardening.
2. Reconcile #94 and #95 into one current-main project-routing implementation; avoid duplicate pipelines.
3. Integrate recording and meters only against that canonical pipeline.
4. Add project-wide transaction/undo.
5. Add timeline/playlists/comping, then automation/MIDI.
6. Add plugin isolation/PDC, export/interchange and post-production.
7. Expand the command registry to cover each implemented feature as it becomes available.

## Source links

- Avid Pro Tools 2026.11 release notes: https://kb.avid.com/pkb/articles/en_US/Knowledge/Pro-Tools-2026-11-Release-Notes
- Avid What's New 2026.11: https://resources.avid.com/SupportFiles/PT/Whats_New_in_Pro_Tools_2026.11.pdf
- Avid Pro Tools Reference Guide 2026.11: https://resources.avid.com/SupportFiles/PT/Pro_Tools_Reference_Guide_2026.11.pdf
- Ardour session: https://github.com/Ardour/ardour/blob/master/libs/ardour/ardour/session.h
- Ardour playlist: https://github.com/Ardour/ardour/blob/master/libs/ardour/ardour/playlist.h
- Ardour automation: https://github.com/Ardour/ardour/blob/master/libs/ardour/ardour/automation_list.h
- Ardour track: https://github.com/Ardour/ardour/blob/master/libs/ardour/ardour/track.h
- Ardour route: https://github.com/Ardour/ardour/blob/master/libs/ardour/ardour/route.h
- Ardour MIDI track: https://github.com/Ardour/ardour/blob/master/libs/ardour/ardour/midi_track.h
- Ardour export handler: https://github.com/Ardour/ardour/blob/master/libs/ardour/ardour/export_handler.h
- Ardour control protocol: https://github.com/Ardour/ardour/blob/master/libs/ctrl-interface/control_protocol/control_protocol/control_protocol.h
- Ardour license: https://github.com/Ardour/ardour/blob/master/COPYING
