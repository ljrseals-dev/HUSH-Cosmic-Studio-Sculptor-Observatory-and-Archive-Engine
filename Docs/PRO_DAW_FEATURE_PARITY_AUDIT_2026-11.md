# HUSH Professional DAW Feature-Parity Audit — 2026-11

## Executive result

Baseline reviewed: main at 8e4b16af1e4fd9bd18f7b2c1c55e664b8dbfaa43 (2026-11-17).

**Feature-parity score: 30/100 (F for professional-DAW feature coverage).** This is not a code-quality grade. It measures the completeness of end-to-end professional DAW workflows against a Pro Tools Studio-class baseline, selected Ultimate/post capabilities, and repeatedly requested cross-DAW workflows. HUSH is an audio-engine and mixer foundation, not a finished DAW. The score intentionally does not count unmerged pull requests as shipped features.

HUSH has valuable engineering foundations: C++20, project persistence, recovery journal, realtime queues, a bounded mixer, bus graph and renderer, ALSA host, Qt mixer, transport/tempo foundations, MIDI CC mapping, WAV media, and recording primitives. The primary gap is end-to-end integration: many components do not yet form a user-visible, persisted, undoable, realtime-tested workflow.

## Scoring rubric

| Capability family | Weight | Main score | Evidence-based reason |
|---|---:|---:|---|
| Session model, persistence, recovery | 10 | 8 | Schema-v1 JSON, stable track/bus/clip IDs, journal and restart recovery; broader entities and host lifecycle remain incomplete |
| Realtime engine and audio I/O | 15 | 7 | Preallocated processing, SPSC queues, ALSA backend and adapters; complete project playback/recording path and target-device qualification remain open |
| Mixer, routing, metering | 15 | 4 | Track mixer, bus graph and routing stage exist; complete persisted project-to-live master path, sends/inserts, full mute/solo and graph-wide meters are not on main |
| Arrangement and editing | 10 | 1 | Sample-positioned clip model and offline renderer; no full arrangement/timeline editor or editing tool workflow |
| Recording, takes, comping | 10 | 2 | Capture ring, WAV writer/session primitives; no complete track-arm, take/playlist, punch, comp and latency-compensation workflow |
| MIDI and instruments | 10 | 1 | Bounded MIDI CC learn mapping; no MIDI track/event model, device I/O, piano roll, instrument routing, MPE or expression maps |
| Tempo and automation | 5 | 2 | Transport/tempo-map and step/linear automation primitives; automation is not a complete persisted, editable, realtime parameter system |
| Plugins, inserts and delay compensation | 10 | 0 | No production plugin host; VST3 SDK gate is documentation only; plugin sandbox, state, scanning and PDC are absent |
| Media import, export and interchange | 5 | 2 | WAV import/export and offline bus rendering; batch stem queue, AAF/OMF/RPP interchange, FLAC and session transfer absent |
| Post, video, spatial and monitoring | 5 | 0 | No timecode/video/ADR, surround/Atmos, Control Room, field-recorder or immersive monitoring path |
| Commands, shortcuts and workflow | 5 | 0 | Current main mixer has buttons but no central searchable command catalogue or keyboard command registry |
| Validation and performance evidence | 5 | 3 | Broad contract/CI coverage and synthetic harnesses; no qualifying target-device, 500-track, 8-hour or release evidence |
| **Total** | **100** | **30** | **Foundation, not professional-DAW parity** |

## What is actually on main

### Present foundations

- Project JSON schema v1 with tracks, buses, routes, media-source metadata and sample-positioned audio clips.
- Bus-graph validation, cycle detection, offline/reference rendering and WAV export.
- Realtime track-mixer and track-to-bus router primitives, SPSC parameter queues, preallocated processing and graph publication.
- Optional Linux ALSA backend/host and optional Qt 6 mixer GUI.
- Undo/redo primitives for track gain, pan, mute and solo.
- Recovery journal, autosave policy, WAV reader/writer, capture ring and recording-session components.
- Transport play/stop/pause/seek and tempo-map conversion.
- Step/linear automation lane and bounded MIDI CC learn mapping.

### Important gaps in the actual model/UI

- Project model does not yet represent MIDI tracks/events, playlists/takes, marker lanes, track/clip folders, plugin instances/state, mix snapshots, expression maps, video, or immersive channel layouts.
- OperationLog only models SetGain, SetPan, SetMute and SetSolo. It is not a complete project-wide transaction/undo system; deleting a track and restoring all dependent data is not represented.
- AutomationLane supports Step and Linear interpolation only and is not a first-class persisted project entity.
- MidiLearnMap maps CC values to numeric parameter IDs; it does not enumerate/open MIDI devices or implement MIDI note/event editing.
- The Qt application is a compact mixer shell. It does not provide an arrangement editor, piano roll, plugin manager/editor, clip browser, session tabs or a professional export queue.
- The ALSA GUI monitor currently maps capture L/R to tracks 1–2. It is not a complete multi-input, project-aware recording/playback workflow.

## Pro Tools-class feature comparison

The reference baseline is Pro Tools Studio with selected Ultimate/post workflows, not a promise to clone proprietary internals or copy its licensing model.

| Pro workflow | Pro Tools reference capability | HUSH main status | Required HUSH work |
|---|---|---|---|
| Large sessions | Hundreds of audio, MIDI, aux, folder and VCA tracks | Partial track model/mixer capacity; no full DAW session workflows | Bounded track/folder hierarchy, stress-tested persistence and UI virtualization |
| Recording | Loop record, playlists, comping, punch | Recording primitives only | Device capture wiring, arm state, take IDs, playlists, punch, comp and latency measurement |
| Editing | Slip/Grid/Shuffle/Spot, Smart Tool, clip gain, fades, nudge, elastic audio | Clip metadata/offline render only | Arrangement canvas, edit modes, transactional clip tools, sample-accurate snapping/nudge, stretch |
| MIDI | MIDI tracks/editor, instruments, MIDI effects, audio-to-MIDI, MPE | CC mapping foundation only | MIDI event model, device backend, piano roll, controller lanes, MPE, expression maps |
| Mixing | Inserts, sends, VCA, advanced automation, AFL/PFL, meters | Basic mixer and bus foundations | Complete project graph, inserts/sends, VCA groups, monitoring buses, PFL/AFL, PDC and metering |
| Plugins | AAX ecosystem, plugin state, scan, latency compensation, freeze/commit | No production host | Licensed SDK decision, sandbox process, IPC, crash isolation, plugin state, scanner, PDC |
| Automation | Track/plugin automation and advanced modes | Step/linear lane primitive | Persisted automation entities, read/touch/latch/write, Bézier/S-curve, parameter delivery |
| Delivery | Stem/multi-stem export, selected-range export, AAF/OMF | WAV export foundation | Export queue, naming templates, range/cue batching, stem grouping, AAF/OMF/RPP converters |
| Post/spatial | Timecode, video, ADR/reconform, surround/Atmos, immersive | Missing | Timebase/timecode, video abstraction, channel-layout model, multichannel processing/rendering |
| Workflow | Search, shortcuts, batch commands, templates, workspaces | Missing on main | Central command registry, searchable palette, keymap profiles, macros, workspace presets |

## User-request research and recommended integrations

Research sources reviewed include the Pro Tools 2026.11 reference guide and feature comparison, the September–November 2026 VI-CONTROL “Your Pro Tools 2027 Wish List” discussion (pages 1–6), Reddit's “PT Wishlist Features/Changes” thread, and Production Expert's 2026 missing-features article. Forum requests are qualitative signals, not a statistically representative survey. Items that already exist in Pro Tools are not represented as missing Pro Tools features; they are considered only as possible HUSH workflows.

### Repeated or high-value requests

| Request seen in user discussions | HUSH status | Recommendation |
|---|---|---|
| All commands searchable and keyboard-addressable | Missing on main; command-palette implementation is on this branch | Make every enabled QAction originate from a stable command ID; palette, menu and shortcut share one action |
| Complete undo, including track deletion and dependent state | Partial | Replace four scalar operation kinds with typed, transactional project operations; delete/restore track, clips, automation, routes, inserts and plugin state atomically |
| Mix snapshots / A-B recall | Missing | Add named, selective snapshots of mixer state; preview/recall must be undoable and must not mutate the audio callback directly |
| Better take/playlist display and fast playlist navigation | Missing | Add explicit take UUIDs, playlist membership, comp ranges, take colors, next/previous playlist commands and global solo/unsolo |
| MIDI CC lanes and editable curves | Missing end-to-end | Promote MIDI CC/velocity/expression to sample/beat-clocked lanes; support Bézier and shaped curves with undo and persistence |
| Expression maps / articulation sets | Missing | Model articulation IDs, keyswitches, controller conditions, per-articulation delay and instrument-map presets |
| MIDI editing in the main timeline / multi-track piano roll | Missing | Use one event model across arrangement and piano-roll views; support linked selection and non-destructive multi-track editing |
| MIDI MPE | Missing | Add per-note pitch/pressure/timbre expression, channel allocation and deterministic event scheduling |
| Multiple sessions and cross-session copy | Missing | Introduce a session manager and explicit import/transfer transactions; do not share live engine objects between sessions |
| Batch export queue | Missing | Named export jobs over ranges/markers/stems, deterministic filenames, collision policy, cancel/resume and reports |
| Clip-list folders and track folders | Missing | Add stable folder entities and hierarchy with cycle checks, collapse state as view state, and reversible moves |
| Plugin favorites, custom groups and hide-unused | Missing | Build plugin catalogue metadata and local user collections after safe plugin scanning exists |
| Control Room / reference monitoring | Missing | Add monitor buses, reference switching, mono, dim, mute and speaker-layout profiles outside the master render graph |
| Batch spot/align to timecode and marker lanes | Missing | Add timecode-aware clip placement, named marker tracks and batch alignment transactions |
| Audio clip envelopes / modulators | Missing | Add clip-relative envelopes and deterministic modulation sources; never run arbitrary scripting in the audio callback |
| Clip-list search/preview and smarter session import matching | Missing | Add local media indexing, waveform preview, metadata matching suggestions and an explicit user-confirmed import plan |
| Playback-device switching without restarting the app | Missing | Separate device lifecycle from session state; stop/quiesce, close, reconfigure and restart safely with rollback |
| Multiple sessions open / compare mix revisions | Missing | Session tabs and mix snapshots; no hidden project state or implicit file overwrites |
| Session versioning / cue containers / snapshots | Missing | Add explicit project branches or cue/sub-session containers only after stable project transactions and media ownership; keep snapshot recall separate from destructive history |
| Multiple tempo/meter maps and per-cue markers | Partial tempo map only | Add named tempo/meter map variants, per-cue timebases and multi-select marker operations with sample/SMPTE-safe conversions |
| Track presets and folder-derived groups | Missing | Save/recall full track templates and create undoable groups from routing-folder membership |
| Control-surface auto-banking | Missing | Add a documented, local control-surface protocol layer (e.g. HUI/Mackie where appropriate), selection-follow and bank synchronization |
| Quick-swipe comping | Missing | Make take audition/comp selection a direct gesture with explicit listen/monitor state and reversible comp ranges |
| Chord track and chord detection | Missing | Add a shared chord-event model and optional local analysis; MIDI arrangement and piano roll must consume the same chord data |
| Local stem separation | Missing | Optional preinstalled local model with explicit user action, offline-only inference and no callback/model work on the realtime thread |
| Versioned track presets / shared global racks | Missing | Add preset library and explicit shared-vs-local override semantics after plugin state and multi-session ownership are defined |

### Important corrections from the discussions

- Some forum suggestions were already available in the referenced Pro Tools version, or were disputed by other users. Treat them as workflow requests, not verified product defects.
- Some requests have third-party workarounds (plugin managers, bounce utilities, macros). HUSH should implement local first-party equivalents only where they fit the product contract.
- Requests for cloud-linked extensions, telemetry, online sample libraries or remote LLM calls are not adopted by default. HUSH remains offline-first; optional local models must be preinstalled and run outside the audio callback.
- Forum popularity is not a substitute for architecture review. A feature is accepted only when its state, threading, persistence, undo, error and test contracts are defined.

## Cross-component transition audit

A DAW feature is complete only when the following transitions are demonstrated:

1. **User intent → command:** stable command ID, discoverable menu/palette entry, documented shortcut.
2. **Command → domain transaction:** validated project mutation; failure leaves model and history unchanged.
3. **Transaction → undo/redo:** full before/after state, stable entity identity, deterministic replay.
4. **Project → persistence:** save/load round-trip, explicit schema migration, no hidden view-state mutation.
5. **Project → realtime engine:** control-thread preparation and bounded queue/publication; audio callback does not allocate, block, log or perform I/O.
6. **Realtime engine → observation:** meters/status are polled from lock-free snapshots; no GUI calls from the audio thread.
7. **Recovery → consistent state:** interrupted writes and process restart recover the last valid durable state.
8. **Offline render → reference:** sample-accurate deterministic output for the same project and media.
9. **Failure → user feedback:** structured error, no partial state mutation, retry/rollback path.
10. **Evidence → release claim:** exact commit, CI run, configuration and hardware report are recorded.

Current examples of broken or incomplete transitions: the track-to-bus router is not connected to persisted project topology on main; automation and tempo primitives are not a complete persisted automation system; recording primitives are not a complete device-to-take workflow; and the Qt mixer has no central command catalogue on main.

## Open pull requests and integration hazards

These are not counted as shipped features. A comparison against current main showed:

| PR | Branch | Ahead / behind main | Finding |
|---|---|---:|---|
| #98 | fix/hush-persistence-recovery-and-realtime-tests | 10 / 0 | Current-base JSON/recovery/test hardening; prioritize review and CI |
| #94 | feat/readme-open-implementation-gates | 175 / 10 | Large project mixer, autosave/recovery, bus controls/inserts and capture work; heavily diverged and overlaps #95 |
| #95 | feat/project-integrated-realtime-mixer | 17 / 10 | Project-aware realtime mixer foundation; draft and diverged |
| #70 | work/clip-track-mixer-wiring | 13 / 94 | Offline clip-to-bus integration; substantially behind main |
| #74 | feat/hush-0.4a-recording-foundation | — | Recording foundation is still open; do not call complete |
| #78 | feat/hush-max-tracks | — | Track capacity/structural controls still open |
| #81 / #84 | live meter branches | — | Live output/per-track RMS/peak integration remains open |

Recommended integration order:
1. Review and validate #98 against current main.
2. Reconcile #94 and #95 into one current-base project-mixer implementation; do not merge both blindly or assume either is on main.
3. Rebase/re-evaluate #70 after the canonical project mixer and clip path are agreed.
4. Complete recording, meters, autosave and performance evidence as separate acceptance slices.
5. Only then build arrangement, MIDI, plugin and delivery layers on the stable engine path.

## Prioritized HUSH integration plan

### P0 — Stabilize and complete the current foundation
- Review #98; fresh default, ALSA, Qt, Qt+ALSA, ASan, TSan and artifact-hygiene CI.
- Reconcile the project-aware realtime mixer and persisted routing; one canonical branch, no duplicate parallel pipeline.
- Integrate recording worker, track arming, source assignment, take identity and recovery lifecycle.
- Finish graph-wide meter/xrun snapshots and 500-track / 8-hour / physical-device evidence.
- Preserve offline build and privacy contracts.

### P1 — Workflow command system
- Central command IDs and action registry.
- Searchable command palette; every enabled command appears in search.
- Keyboard shortcuts for every implemented command; no hidden menu-only actions.
- Context-aware selected-track commands and platform keymap profiles.
- Next: user-editable shortcuts, conflict detection, import/export keymaps and local macros.
- All commands route through the same domain transaction and undo path.

### P2 — Arrangement and media management
- Timeline view, selection, snapping, edit modes, trim/move/nudge, fades, clip gain and non-destructive stretch.
- Track/clip folders, marker lanes, clip-list folders, media search and waveform audition.
- Sample-accurate undoable operations and view-state persistence separate from project state.

### P3 — Recording and comping
- Device inputs and track arming, take IDs, playlists, loop/punch recording, comp ranges, crossfades and latency compensation.
- Fast playlist navigation and take colors.
- 8-track × 10-minute test and named-device loopback evidence.

### P4 — Automation and MIDI
- Persisted automation entities with read/touch/latch/write, Bézier/S-curve and controller lanes.
- MIDI event tracks, SMF type 0/1 import/export, piano roll, MIDI CC/velocity/expression lanes, MPE and expression maps.
- Shared event model for arrangement and piano roll.

### P5 — Professional mixer and plugin system
- Sends, inserts, VCA, Control Room, AFL/PFL, mono/dim/reference switching, multichannel layouts and delay compensation.
- VST3 SDK/license/provenance gate; out-of-process plugin host, watchdog, state round-trip, crash containment and scanner.
- Plugin favorites/custom folders/hide-unused only after a safe catalogue exists.

### P6 — Delivery and post-production
- Batch export queue, stem/range/marker naming templates, render reports and cancellation/retry.
- AAF/OMF and documented best-effort REAPER RPP interchange; do not invent a proprietary PTX parser.
- Timecode/video, field-recorder workflows, reconform, surround and immersive rendering as separately gated milestones.

### P7 — Local intelligence, optional platform parity
- Optional local stem separation, transcription and media matching only with preinstalled model assets and explicit user action.
- No cloud calls, telemetry, automatic downloads or remote services in the default product.
- Windows/macOS backends after OS abstractions and parity tests are in place.

## Release policy

- Never count a design document, open PR, branch-only implementation or smoke test as a shipped feature.
- Do not claim plugin hosting, full recording, MIDI production, arrangement editing, immersive mixing, zero-xrun operation or release readiness until the full end-to-end gates pass.
- Preserve C++20, offline builds, explicit optional dependencies, no telemetry, no call-home behavior, and no allocation/locking/logging/I/O in realtime callbacks.
- Update this audit's score only against the current main commit and the exact evidence attached to it.

## Sources

- Avid Pro Tools 2026.11 Reference Guide: https://resources.avid.com/SupportFiles/PT/Pro_Tools_Reference_Guide_2026.11.pdf
- Avid Pro Tools feature comparison: https://www.avid.com/pro-tools/comparison
- Avid Pro Tools current features: https://www.avid.com/pro-tools
- Avid What's New: https://www.avid.com/pro-tools/whats-new
- Avid Pro Tools 2026.11 release notes: https://kb.avid.com/pkb/articles/en_US/Knowledge/Pro-Tools-2026-11-Release-Notes
- Pro Tools Shortcuts Guide: https://resources.avid.com/SupportFiles/PT/Pro_Tools_Shortcuts_2025.6.pdf
- VI-CONTROL, Your Pro Tools 2027 Wish List (Sep 2026): https://vi-control.net/community/threads/your-pro-tools-2027-wish-list.174855/
- VI-CONTROL wishlist page 4 (session versioning, track presets, routing-folder groups, control surfaces): https://vi-control.net/community/threads/your-pro-tools-2027-wish-list.174855/page-4
- VI-CONTROL wishlist page 5 (stem separation, sub-sessions, tempo/meter playlists, plugin sandboxing, comping): https://vi-control.net/community/threads/your-pro-tools-2027-wish-list.174855/page-5
- VI-CONTROL wishlist page 6 (follow-up discussion on tempo maps and MIDI workflows): https://vi-control.net/community/threads/your-pro-tools-2027-wish-list.174855/page-6
- Reddit, PT Wishlist Features/Changes: https://www.reddit.com/r/protools/comments/1lfxtbx/
- Reddit, What's Still Missing In Pro Tools?: https://www.reddit.com/r/protools/comments/f5v29m/
- Reddit, Why is there no insert undo feature?: https://www.reddit.com/r/protools/comments/1n9ndr3/
- Production Expert, 3 Pro Tools Features Still Missing In 2026: https://www.production-expert.com/production-expert-1/3-pro-tools-features-still-missing-in-2026
