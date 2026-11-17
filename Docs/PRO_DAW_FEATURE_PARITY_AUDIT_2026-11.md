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

## Current Pro Tools 2026.11 delta

The latest official Avid release documentation available for this review is Pro Tools 2026.11 (released November 3, 2026). It adds several parity targets that must be explicitly represented rather than relying on older Studio feature matrices:

| Pro Tools 2026.11 capability | HUSH status in reviewed main | HUSH integration target |
|---|---|---|
| Pro Tools Extensions: session-wide and no-session tools, third-party integrations and organized side-panel tabs | Missing | Define a HUSH extension contract only after process isolation, capability permissions, versioned API and offline package verification exist; never load arbitrary code in the host/audio callback |
| Pro Tools Stem Separator: local AI separation into Vocals, Drums, Bass, Guitar, Keys and Other, with preview and track creation | Missing | Optional preinstalled local model, explicit offline job, progress/cancel, preview, generated-media provenance and transactional import; model inference stays out of realtime |
| MIDI Polyphonic Expression (MPE): per-note pitch, timbre and pressure/amplitude expression | Missing | Per-note expression event model, channel allocation, editor lanes, device input/output, persistence, undo and deterministic scheduler |
| Alternate-take auto-coloring | Missing | Stable take/playlist identity and deterministic palette assignment; color is metadata and must not change take ordering or comp ranges |
| Focus switching between pinned and non-pinned track panes | Missing | Add focus navigation commands to the same command registry and keymap; preserve accessible focus and selected-track context |
| Import/export and UI customization enhancements | Partial | Complete import/export job queue and user-persisted workspace/keymap profiles with conflict handling |

These are confirmed release capabilities, not forum speculation. The official release notes also document local processing for Stem Separator; HUSH should preserve the same offline privacy boundary rather than assuming cloud inference. `Pro Tools Extensions` is a useful extensibility concept, but HUSH should not adopt a plugin/extension loader until sandboxing, signing/provenance, permissions, ABI stability and crash recovery are implemented. [Avid 2026.11 release notes](https://kb.avid.com/pkb/articles/en_US/Knowledge/Pro-Tools-2026-11-Release-Notes) · [What's New 2026.11](https://resources.avid.com/SupportFiles/PT/Whats_New_in_Pro_Tools_2026.11.pdf).

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


## Forensic refresh — 2026-11-17 12:05 UTC

This refresh re-checked the live repository after the initial audit. The default branch remains `main` at `8e4b16af1e4fd9bd18f7b2c1c55e664b8dbfaa43`; therefore the main-branch score above remains **30/100**. The repository's latest push is to a feature branch, not a change to main.

### Exact branch and validation state

| PR | Head | Main comparison | Current state | Disposition |
|---|---|---:|---|---|
| #98 persistence/recovery hardening | `7c3861805fe5e2e95acd06fb9d27ae21c96a1bc1` | 17 ahead / 0 behind | Open; 8/8 checks successful | Review and merge first; includes strict project JSON parsing/recovery fixes |
| #99 DAW parity / command palette | `8b9bad2101a701fb187809f04642efabf9610582` | 12 ahead / 0 behind | Draft open; 8/8 check runs successful | Keep draft until command behavior and integration scope are reviewed |
| #94 project-aware mixer/autosave/recording | `58da43d619df6af21378c07fc328dbea6ddce6ae` | 175 ahead / 10 behind | Open; GitHub reports `dirty` | Do not merge as-is; broad 28-file integration branch conflicts with current main |
| #95 project-integrated realtime mixer | `f1e2e0f81fcfebf9b5e2397a898c7e6d413ca576` | 17 ahead / 10 behind | Draft open; GitHub reports `dirty` | Do not merge as-is; extract/reconcile its runtime composition with #94 and current main |

For #99, the current exact head has eight successful check runs: seven jobs in Actions run [44079263421](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/44079263421) and the package job in [44079263362](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/44079263362). The earlier PR-body statement that workflows did not trigger on this docs-only head is stale and must be corrected. These are CI results, not physical ALSA, latency, xrun, long-duration soak, or release-readiness evidence.

### Source-level integration facts reconfirmed on main

- `Track` in `include/hush/project.hpp` has identity/name/gain/pan/mute/solo but no persisted bus assignment. The project has bus and route arrays, so the missing track-to-bus edge prevents the stored graph from fully describing the live track routing topology.
- The ALSA Qt `LiveMixer` in `src/mixer_gui.cpp` maps the input's left/right channels only to source tracks 0 and 1; remaining tracks are zero-filled. This is a monitor/mixer test path, not full multichannel media playback or project recording.
- `OperationLog` supports only SetGain, SetPan, SetMute and SetSolo. It cannot undo track/clip deletion, route/insert edits, recording takes, automation, or session-wide snapshots as a single project transaction.
- `AutomationLane` is a standalone Step/Linear evaluator; no persisted automation entity or complete parameter-delivery path is represented in the main project schema.
- `MidiLearnMap` maps CC events to numeric parameter IDs but explicitly does not enumerate/open devices or apply parameters; it is not a MIDI track/event engine or MPE implementation.
- The command palette branch now has a real shared QAction/menu/shortcut/palette path and offscreen Qt assertions. It still does not provide editable keymaps, macros, timeline/MIDI/transport command coverage, or project-wide transactional undo. Do not award the full command-system category.

### Damage / merge-risk conclusion

No default-branch mutation was observed. The principal damage risk is integration by cherry-picking or merging overlapping stale branches without reconciling their changes to `project.hpp`, `project.cpp`, `operation_log.*`, `mixer_gui.cpp`, `CMakeLists.txt`, and shared test/acceptance documents. #94 and #95 both touch the project mixer/runtime boundary; #94 is the broader implementation candidate, while #95 is a smaller isolated runtime composition candidate. They must be compared at the code-hunk and contract level before either is integrated. #98 is the current-base safety fix and should precede feature integration.

Recommended order is #98 first; then create one current-main project-mixer integration branch that reconciles the best validated pieces of #94/#95, persists track-to-bus assignment, wires the same pipeline into Qt and ALSA, and proves model → transaction/undo → persistence/recovery → bounded realtime publication → meter observation. Only after that should recording/takes, automation/MIDI, plugin hosting/PDC, and post/delivery be layered on.

The grade remains main-only and evidence-bound. A feature in an open PR, a protocol document, or a passing synthetic unit test does not count as shipped end-to-end functionality.


## Additional current user-request evidence — November 2026

A current VI-CONTROL Pro Tools 2027 wishlist thread adds several concrete workflow requests and corrections. These posts are qualitative evidence from individual users, not a statistically representative survey, and some requests are disputed or already available in certain Pro Tools configurations. HUSH should implement the underlying workflow where useful, not copy proprietary UI or command implementations.

| Reported request | HUSH status in reviewed main | Recommended HUSH implementation / transition |
|---|---|---|
| Control Room-style monitoring | Missing | Dedicated monitor-bus domain model with reference switching, mono, dim, mute and speaker layouts; do not route monitor-only controls into exported master audio |
| Multichannel plugin instances and immersive processing | Missing | Channel-layout-aware bus/insert contract, explicit channel maps, validated downmix/upmix matrices, and plugin channel capability negotiation before plugin host work |
| Batch processing and batch export | Missing | Persistent export-job model, deterministic queue, per-job output paths, collision policy, cancel/resume, error report and reproducible render hash |
| MIDI CC curves editable with Bézier and direct curve transforms | Missing end-to-end | Persisted automation/event curves with interpolation types, selection transforms (compress/expand/crescendo), sample/beat domain conversion and undo |
| Expression maps / articulation sets | Missing | Stable articulation IDs, keyswitch/controller conditions, articulation presets, instrument routing and serialized mapping version |
| Undo deletion of tracks | Missing | One atomic delete transaction capturing track, clips, playlists, routes, sends, automation, plugin state and references; undo restores exact stable IDs and ordering |
| Search/palette/shortcut coverage gaps | Partial on PR #99 only | Registry must enumerate every enabled command; add tests that every command is discoverable, shortcut conflicts are surfaced, and context-sensitive commands are disabled when unavailable |
| Multi-track piano roll and MIDI controller lanes in Edit view | Missing | Shared MIDI event model and selection across arrangement/piano-roll views; CC/velocity/expression lanes must edit the same persisted events |
| Fine audio/MIDI nudge and spacing commands | Missing | Integer sample/tick operations with explicit units, snapping policy, keyboard-repeat behavior, undo grouping and overflow tests |
| Better session-data import matching / preview | Missing | Local metadata extraction, candidate match report, source timeline/tempo preview and explicit confirmation before import transaction |
| FLAC import and drag/drop MIDI exchange | Missing | Format capability registry, offline decoder, explicit media import transaction and safe MIDI file/clipboard interchange; preserve provenance |
| Plugin favorites/hide-unused and AudioSuite-style offline chains | Missing | Local plugin catalogue metadata and user collections; offline processing chain only after plugin isolation, state capture, deterministic render and rollback exist |

The current thread also includes a report of a MIDI Operations selection regression. This is a Pro Tools bug report, not a feature HUSH should reproduce; HUSH's acceptance tests should ensure invoking an operation preserves the selected event set and does not require re-selection.

Sources: [VI-CONTROL Pro Tools 2027 wishlist, page 1](https://vi-control.net/community/threads/your-pro-tools-2027-wish-list.174855/), [page 2](https://vi-control.net/community/threads/your-pro-tools-2027-wish-list.174855/page-2), [page 3](https://vi-control.net/community/threads/your-pro-tools-2027-wish-list.174855/page-3). Official shortcut behavior and command-focus semantics should be taken from the [Pro Tools Reference Guide 2026.11](https://resources.avid.com/SupportFiles/PT/Pro_Tools_Reference_Guide_2026.11.pdf), not inferred from forum comments.

## Command and protocol completeness gate

The current PR #99 command palette is a useful first slice, not a complete command system. It registers only the commands listed in `Docs/WORKFLOW_COMMANDS.md`. Do not count a shortcut or protocol as delivered until all of these are true:

1. Stable command ID and versioned argument schema exist.
2. Menu, shortcut, palette, macro and external control-surface entry resolve to the same command handler.
3. Context/permission checks disable unavailable commands and prevent shortcuts firing while text-entry widgets own focus.
4. Handler validates input and applies a single project transaction.
5. Undo/redo restores exact state, including dependent entities.
6. Save/reopen and recovery preserve the result.
7. Realtime publication is bounded and occurs at a block boundary; callbacks never call GUI or agent code.
8. UI and control surfaces receive coherent state feedback.
9. Invalid requests and queue-full conditions fail without project/engine divergence.
10. Tests exercise menu, shortcut, palette and protocol invocation against the same domain transaction.

External control protocols should be introduced in layers: first an internal command bus and event/result schema, then OSC/MIDI control-surface adapters with explicit capability maps and feedback, then optional transport synchronization (MIDI Clock, MTC/LTC) only after the timebase and transport state machine are stable. Do not add protocol labels to menus without implementing transport, feedback, lifecycle, authorization and failure semantics.


## Integration checkpoint — 2026-11-17 (updated after current-main integrations)

The original 30/100 (F) result remains the reproducible historical baseline against commit `8e4b16af1e4fd9bd18f7b2c1c55e664b8dbfaa43`; it is not a fresh grade of the current main. Do not add points for a feature foundation that is not a complete user-visible workflow.

### Current main changes

- PR #98 merged as `2bbc9bc77c23e1ba678223f9b58a8ab0b38cdb2e`: strict JSON parsing, locale-independent numeric handling, duplicate ID checks, recovery-journal continuation and realtime graph test.
- PR #100 merged as `68b7b09816b89e0b7f5c96a4ae27f8524fc23c08`: shared Qt command registration for Open/Save/Undo/Redo, selected-track Mute/Solo, optional ALSA live toggle and searchable Ctrl+K command palette. All eight CI checks passed.
- PR #102 merged as `dfdd6b6cbafbcc006e2f633cec673ebae28f8176`: note-scoped MPE pitch/pressure/timbre model, strict validation and allocation-free expression interpolation. All eight CI checks passed. This is not yet a complete MIDI/MPE editor or playback path.
- PR #103 is open against current main: persists optional `take_id` and `take_number` on audio clips, validates shared take-number consistency, and provides deterministic color-index mapping. The recording coordinator does not yet assign take IDs automatically; playlist/comping and undo/redo integration remain open. CI is in progress.

### Remaining high-priority Pro Tools 2026.11 transitions

| Requirement | Current state | Required completion |
|---|---|---|
| MPE record/edit/play/automate | Core per-note data/evaluation merged | Project persistence, MPE zone/device input, scheduler, MIDI editor expression lanes, channel color rendering, instrument delivery, undo/recovery/offline-render tests |
| Auto-color alternate takes | Clip identity PR #103 pending | Recording coordinator assigns one take ID to every track in a simultaneous pass; playlist edits preserve it; comping UI and tests |
| Pinned/unpinned track focus shortcut | Not implemented | Pinned-track model/view, focus state machine, context-safe command and keyboard test |
| Local six-stem separation | Not implemented | Optional offline inference engine, locally stored model weights with provenance/license, six-stem output contract, preview/import transactions, resource limits and no-network tests |
| Session-independent Extensions | Not implemented | Local extension manifest/registry, lifecycle and capability model, safe process boundary, extension menu/panel and no-session host |
| Project-aware mixer/recording | PRs #94/#95 remain conflicted integration candidates | Selectively port track-to-bus persistence, inserts, meters, routing, recording and recovery; do not merge stale branch history wholesale |

### Current command protocol state

PR #100 delivers the first shared QAction/menu/shortcut/palette layer. It does not deliver editable keymap profiles, shortcut conflict detection, macros, a public command protocol, control-surface feedback, or project-wide structural undo. Commands must still follow the complete transaction contract above; menu presence alone is not implementation.

### Community-requested workflow priorities

The VI-CONTROL 2027 wishlist and Pro Tools user discussions repeatedly mention expression maps/articulations, MIDI CC automation curves, multitrack piano-roll editing, undoing track deletion, batch processing/export, import preview, fine nudge, and better command search/shortcut coverage. Reddit users also request a persistent undo history, timestamp-aware audio import, batch fades and playlist breakout. These are qualitative user requests, not a statistically representative survey; they should guide HUSH's workflow roadmap, not be misrepresented as universal consensus.

Sources: [Avid Pro Tools 2026.11 release notes](https://kb.avid.com/pkb/articles/en_US/Knowledge/Pro-Tools-2026-11-Release-Notes?popup=true), [VI-CONTROL wishlist page 1](https://vi-control.net/community/threads/your-pro-tools-2027-wish-list.174855/), [page 2](https://vi-control.net/community/threads/your-pro-tools-2027-wish-list.174855/page-2), [page 3](https://vi-control.net/community/threads/your-pro-tools-2027-wish-list.174855/page-3), [Reddit user requests](https://www.reddit.com/r/protools/comments/1bwhutb).

The score is intentionally not recalculated in this checkpoint: the MPE and take work are still partial workflows, and mixer/recording, Extensions, stem separation and arrangement remain open. Re-score only after the end-to-end transitions and CI evidence close.
