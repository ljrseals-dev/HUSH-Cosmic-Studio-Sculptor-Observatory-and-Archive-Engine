# Proposal: Transport, MIDI, and Automation

Status: **Phase 0 design artifact; implementation requires owner review of the decisions below.** This document does not claim that transport, MIDI, or automation is complete.

## 1. Purpose and scope

The 0.2 tree contains a project model, SPSC parameter queue, realtime track-mixer primitive, and operation-log primitives. It does not yet contain a transport clock, MIDI input/learn, persisted automation lanes, or an automation playback scheduler. This proposal defines an incremental path to add them without weakening the offline/privacy/realtime contract.

Excluded from this proposal: audio recording/takes, plugin hosting, voice control, network synchronization, external clock following in the first implementation, MIDI SysEx, MPE, and power-loss durability claims.

## 2. Transport and tempo

**Recommendation for first implementation:** an internal sample counter advanced by the audio processing callback. It is deterministic and does not require wall-clock timers or external services. The GUI may request play/stop/seek, but the callback owns the authoritative running sample position.

Accuracy target: sample-position state is integer-sample accurate within the engine. A command received through the current block-boundary SPSC queue may take effect at the next block boundary; command scheduling is therefore block-quantized until a timestamped transport-event queue exists. Do not describe this first gate as sample-accurate event scheduling.

Persist a tempo map rather than a single tempo so the model does not require another schema break when tempo changes are introduced. Proposed schema-v2 shape:

```json
{
  "transport": { "start_position_samples": 0 },
  "tempo_map": [
    { "position_samples": 0, "bpm": 120.0, "time_sig_num": 4, "time_sig_den": 4 }
  ]
}
```

The persisted start position is a project start/initial cursor, not the live playhead. Live playhead, playing/paused state, and callback sample counter are runtime-only. Schema-v1 loads with an implicit 120 BPM, 4/4 event at sample zero and a zero start position. New saves use schema v2 only after a reader/writer migration gate is implemented and tested; do not change the current v1 reader in the automation-foundation slice.

Transport commands cross the existing bounded SPSC boundary as trivially copyable POD messages. No GUI slot directly changes callback-owned state.

**Click/metronome:** defer until tempo-map and transport tests pass. Generate clicks from preallocated phase/voice state in the audio callback; do not allocate or read files there. Volume/mute are parameters delivered through the queue. Open decisions: count-in, loop-region persistence, and whether transport state is per-project or application-global.

## 3. MIDI input

Backend options:
- **ALSA sequencer (snd_seq):** native Linux option and reasonable first backend, but requires preinstalled ALSA development/runtime support and target-device testing.
- **JACK MIDI:** useful where JACK is already the studio server, but adds an optional dependency and server lifecycle.
- **PipeWire MIDI:** aligns with modern Linux desktops, but API and deployment details should be handled in a separate backend adapter.

Recommendation: keep the first transport/automation gates backend-independent; choose ALSA sequencer as the first optional Linux backend only after owner approval, with no network bootstrap. MIDI arrives on a backend/control thread and is copied into a bounded ring of fixed-size events:

```cpp
struct MidiEvent {
  std::uint64_t timestamp_samples;
  std::uint8_t status, data1, data2;
};
```

The callback consumes only bounded POD events. Timestamp conversion must map backend monotonic timestamps to the engine sample clock using a measured/defined origin. Late events clamp to the current block start and increment a local counter; events beyond the accepted lookahead remain queued or are rejected by a documented bounded policy. Device enumeration/rescan belongs on the GUI/control thread and is explicit, not hidden background polling.

Open decisions: multiple-device merge order and channel filtering. SysEx, MPE, and 14-bit CC are deferred from v1.

## 4. MIDI learn and mapping

Proposed mapping key:
`{source_device_id, channel, message_type, number}` → `{target_kind, target_id, param_id, transform(min,max,curve)}`.

Learn flow: arm a target in the GUI; the next supported matching MIDI event proposes a mapping; the user-visible mapping edit is appended through the operation log; an immutable mapping snapshot becomes active at a safe block boundary. No audio-thread map mutation or string lookup is permitted.

Automation mode semantics:
- **Read:** evaluate existing lanes; in the initial playback gate, this is the only implemented mode.
- **Touch:** write only while the control is actively touched; deferred.
- **Latch:** continue writing the last touched value until stop; deferred.
- **Write:** continuously write while armed and running; deferred pending bounded recording-buffer and undo design.

Latency target after a timestamped MIDI event reaches the engine ring: apply at the event's scheduled sample in the block, or at the next block boundary in the first backend-neutral prototype. The latter worst-case queueing delay is one block plus backend delivery jitter and must be measured, not assumed. MIDI feedback/echo, pickup/takeover, and conflicting mappings require owner decisions.

## 5. Automation lanes

One lane per `(track_id, target_param_id)`; each point is `{position_samples, value, curve}`. The initial implementation slice adds a deterministic lane model with Hold and Linear curves and interpolation tests. Lane editing/normalization is control-thread work. The callback reads an immutable snapshot; publishing and retiring snapshots must use the same safe graph-lifetime/reclamation design as the graph-swap gate before automation is integrated into live processing.

Recording is not part of the first slice. A future record gate must reserve a fixed-capacity point buffer before recording starts, write POD events from the callback without allocation, and flush/convert them on a non-audio thread after the pass or in bounded chunks. Overflow must be visible and must never block the callback.

Proposed precedence: automation is authoritative in Read mode; manual GUI/MIDI writes take control only in a future Touch/Latch/Write mode. Do not implement ambiguous manual override behavior before those modes are approved. Open decisions: curve set, point thinning/quantization, and whether one recording pass is one undo operation or bounded chunks.

## 6. Schema-v2 migration

Proposed additions:

```json
{
  "schema_version": 2,
  "transport": { "start_position_samples": 0 },
  "tempo_map": [
    { "position_samples": 0, "bpm": 120.0, "time_sig_num": 4, "time_sig_den": 4 }
  ],
  "midi_mappings": [
    {
      "source_device_id": "local-device-id",
      "channel": 1,
      "message_type": "cc",
      "number": 7,
      "target_kind": "track",
      "target_id": "track-1",
      "param_id": "gain",
      "transform": { "min": 0.0, "max": 2.0, "curve": "linear" }
    }
  ],
  "automation_lanes": [
    {
      "track_id": "track-1",
      "target_param_id": "gain",
      "points": [
        { "position_samples": 0, "value": 1.0, "curve": "linear" }
      ]
    }
  ]
}
```

Migration rule: v1 documents must continue to load unchanged with defaults for all new fields. The v2 reader/writer must round-trip these fields before any feature writes v2. Unknown-field preservation is not currently guaranteed by the regex-based v1 reader; it must be explicitly implemented and tested or clearly rejected as a compatibility guarantee. Tests required: v1 fixture load, v1→v2 default migration, v2 round-trip, invalid field rejection, and unknown-field policy.

## 7. Realtime contract impact

| Touchpoint | Allocation on callback? | Lock? | I/O? | Mitigation |
|---|---:|---:|---:|---|
| Transport sample counter | No | No | No | Integer counter owned by callback |
| Transport command consumption | No | No | No | Fixed-size SPSC POD |
| Tempo lookup | No | No | No | Prevalidated immutable sorted snapshot |
| Click synthesis | No | No | No | Preallocated phase/state; defer until tested |
| MIDI event consumption | No | No | No | Fixed-capacity ring; POD event |
| MIDI mapping | No | No | No | Immutable pre-resolved mapping snapshot |
| Automation evaluation | No | No | No | Read-only points and bounded binary search |
| Automation recording | No | No | No | Preallocated bounded event buffer; overflow counter |
| Project persistence/undo | Not on callback | No callback lock | Control-thread file I/O only | Operation log and file work stay off callback |
| Snapshot retirement | No | No | No | Requires tested deferred reclamation before integration |

Any implementation that introduces callback allocation, locks, logging, file/network I/O, or unbounded work is a blocker and must be redesigned. Keep `hush_privacy_source_audit` green. Extend its denylist if new network/client/callback-registration APIs are introduced; no such APIs are permitted in these features.

## 8. Milestones and evidence

1. **Transport + tempo/schema skeleton:** unit tests for sample counter and tempo lookup, v1 compatibility tests, command-queue tests. No hardware required. Only update README claims after tests pass.
2. **Metronome/click:** deterministic render tests, mute/volume queue tests, callback allocation checks.
3. **MIDI backend + event ring:** virtual-device loopback where supported, event-order and overflow tests, machine-labelled latency report. Requires Linux backend dependencies.
4. **MIDI learn + persistence:** mapping round-trip and undo/redo tests; mapping-conflict policy documented.
5. **Automation read playback:** interpolation boundary tests and block-render test using immutable lane snapshots.
6. **Automation recording and modes:** fixed-buffer overflow test, recorded-pass test, undo-granularity test, sanitizer run.
7. **GUI integration:** headless Qt tests for controls, transport actions, learn, lane editing and visible queue overflow. GUI commands never mutate callback state directly.

Every gate also requires default build + CTest, relevant optional build, privacy audit, evidence named in README and IMPLEMENTATION_STATUS, and a CHANGELOG entry. Hardware-dependent gates require a human-run report; no synthetic test substitutes for that report.

## 9. Risks and explicit non-claims

- **Transport accuracy vs. GUI responsiveness:** block-boundary commands are simple but not sample-scheduled. Mitigation: separate transport-state control from timestamped event scheduling and state the actual accuracy.
- **MIDI jitter:** backend scheduling and desktop load vary. Mitigation: timestamp events, measure on target hardware, and keep MIDI optional.
- **Automation undo complexity:** recorded points can produce huge operation logs. Mitigation: treat a recording pass as a bounded transaction with explicit overflow and owner-approved undo granularity.

This proposal authorizes no claim of hardware validation, zero-network proof, low latency, zero xruns, or soak stability. A passing source audit is a regression check, not proof of no network activity.

## 10. Owner decision checklist

Before implementation beyond the backend-neutral foundations, the owner must approve:

1. Internal sample-counter transport for v1 (recommended), with block-boundary commands initially?
2. ALSA sequencer as the first optional MIDI backend, with no JACK/PipeWire until separate gates?
3. Read-only automation playback first; defer Touch/Latch/Write recording modes?
4. Schema-v2 migration and unknown-field policy as specified above?
5. Equal-power pan law remains the default and automation values are parameter-domain values (e.g. gain 0–2), not normalized UI percentages?
6. MIDI SysEx, MPE, 14-bit CC, MIDI feedback, and controller takeover remain out of v1?
