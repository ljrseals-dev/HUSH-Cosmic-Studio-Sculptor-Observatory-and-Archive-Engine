# GUI-to-engine wiring audit

Date: 2026-10-19
Branch: `work/phase0-phase1-automation-start`
Scope: source audit of `src/mixer_gui.cpp`, `include/hush/spsc_parameter_queue.hpp`, `include/hush/realtime_track_mixer.hpp`, `src/realtime_track_mixer.cpp`, `include/hush/realtime_bus_engine.hpp`, and `src/realtime_bus_engine.cpp`.

## 1. Interactive widgets in the current Qt shell

| Widget | Current signal/action | Current effect | Enqueues engine command today? |
|---|---|---|---|
| Open Project button | `QPushButton::clicked` → `openProject()` | Opens a user-selected local JSON file, parses it through `from_json`, replaces the in-memory `Project`, clears the operation log, refreshes the table | No |
| Save Project button | `QPushButton::clicked` → `saveProject()` | Writes current project JSON to a user-selected local path | No |
| Undo button | `QPushButton::clicked` → `OperationLog::undo(project_)` | Mutates project model through the operation log, refreshes table | No; does not synchronize a live mixer |
| Redo button | `QPushButton::clicked` → `OperationLog::redo(project_)` | Mutates project model through the operation log, refreshes table | No; does not synchronize a live mixer |
| Gain slider | `QSlider::sliderReleased` | Calls `change(..., SetGain, value/100)`; successful edits append an operation and mutate project state | No |
| Pan slider | `QSlider::sliderReleased` | Calls `change(..., SetPan, value/100)`; successful edits append an operation and mutate project state | No |
| Mute checkbox | `QCheckBox::clicked` | Calls `change(..., SetMute, checked ? 1 : 0)`; successful edits append an operation and mutate project state | No |
| Solo checkbox | `QCheckBox::clicked` | Calls `change(..., SetSolo, checked ? 1 : 0)`; successful edits append an operation and mutate project state | No |
| Track-name and stable-ID cells | Non-editable `QTableWidgetItem` | Display-only | No |
| Menus / transport controls | None present in `src/mixer_gui.cpp` | Not applicable | No |

Important detail: `refresh()` constructs widgets and connects their signals. The widget slots currently call project/operation-log methods only. Undo/redo changes project values but there is no live-engine synchronization path.

## 2. Current queue payloads and effects

`SpscParameterQueue<T, Capacity>` is a generic fixed-capacity queue. It accepts a trivially-copyable payload type selected by its owner; the queue itself does not define a universal command enum.

| Payload | Queue owner / API | Fields | Consumer effect |
|---|---|---|---|
| `TrackControlCommand` | `RealtimeTrackMixer::enqueue_control`, capacity 2048 | `track_index`, `gain`, `pan`, `flags` (bit 0 mute, bit 1 solo) | `RealtimeTrackMixer::process` consumes commands before rendering a block and updates per-track gain/pan/flags. It uses equal-power pan: at center each channel receives approximately -3.01 dB relative to a unity mono input. |
| `BusFaderChange` | `RealtimeBusEngine::enqueue_fader_change`, capacity 1024 | `bus_index`, `gain` | `RealtimeBusEngine::process` consumes commands before processing a block and updates that bus's fader gain. This payload has no pan/mute/solo fields. |

Both engine primitives require one producer and one consumer and are configured before processing starts. The existing Qt executable has neither a configured `RealtimeTrackMixer` instance nor a running audio callback attached to it.

## 3. GUI direct access audit

Observed GUI-to-state calls:
- `openProject()`: local file read and project replacement; resets `OperationLog`.
- `saveProject()`: serializes the project and writes the selected local file.
- `change()`: finds a track in `project_.tracks` and calls `OperationLog::apply_and_append`.
- Undo/redo slots: call `OperationLog::undo/redo(project_)`.
- `refresh()`: reads project state to render the widgets.

No engine or graph object is referenced by `src/mixer_gui.cpp`; no direct graph mutation violation was found because the GUI has no engine/graph connection at all. This is not evidence of correct live-engine wiring; it identifies the missing boundary.

## 4. Required command mapping for a future live binding

| Widget action | Proposed bounded POD | Intended consumer | Block-boundary effect |
|---|---|---|---|
| Gain change | Complete `TrackControlCommand` snapshot for that track | `RealtimeTrackMixer` | Updates track gain |
| Pan change | Complete `TrackControlCommand` snapshot for that track | `RealtimeTrackMixer` | Updates track pan; center uses equal-power (-3 dB per side) law |
| Mute change | Complete `TrackControlCommand` snapshot for that track | `RealtimeTrackMixer` | Updates mute bit |
| Solo change | Complete `TrackControlCommand` snapshot for that track | `RealtimeTrackMixer` | Updates solo bit; mixer derives whether any track is soloed |
| Undo/redo of a track control | Re-enqueue a complete snapshot for every changed control/track | `RealtimeTrackMixer` | Restores live controls at a subsequent processing block; current OperationLog does not emit these snapshots |
| Bus gain (no bus widget currently exists) | `BusFaderChange` | `RealtimeBusEngine` | Updates bus fader at a processing-block boundary |
| Queue full | No retry loop; set a GUI-side drop status after enqueue returns false | GUI only | No engine mutation; report that the change was not applied |

The existing track mixer accepts one complete snapshot per command. GUI actions must not call `process()`, mutate the mixer internals, or mutate a graph. A queue-full status needs a clearly defined policy for reconciling the project model with a rejected live update (for example, roll back the edit or show a pending/live mismatch). Silently leaving project and live state divergent is not acceptable.

## 5. README claims this gate is responsible for

Quoted exactly from README.md under “Not yet complete — do not claim as delivered”:

> **GUI-to-live-engine commands and GUI interaction/CLI parity:** no complete GUI binding test; requires the Qt GUI and engine running together.

This gate may narrow or remove this bullet only after every remaining sub-claim is covered by tests. It does not close graph replacement, complete track-to-bus routing, hardware validation, or soak/performance gates.

## 6. Blocker / safe next step

The current `hush_mixer_gui` target links `hush_core` and Qt Widgets only. It is a project editor, not an audio host. The optional ALSA host is a separate executable and routes its capture input through `RealtimeBusEngine`/an adapter; it does not own the Qt window or instantiate `RealtimeTrackMixer`. The project model currently stores track metadata/control values but does not supply audio clip playback buffers to the track mixer.

Therefore, merely enqueueing snapshots from Qt would enqueue into a mixer that no running callback consumes. Adding a dummy GUI timer or processing from the Qt thread would not meet the live-engine requirement and would violate the intended thread boundary. A correct completion needs an explicit host/lifecycle design that owns the audio backend and configured track mixer, plus a defined source of track audio; then GUI actions can target that host's SPSC producer endpoint. This audit does not claim the gate is closed.
