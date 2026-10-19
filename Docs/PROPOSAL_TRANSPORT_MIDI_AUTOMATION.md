# Proposal: Transport, MIDI, and Automation

**Status:** Proposed; awaiting owner decision.  
**Target baseline:** HUSH Cosmic Studio 0.2 headless foundation.  
**This document is not implementation authorization.**

## 1. Purpose and scope

The current 0.2 tree does not contain an integrated transport/tempo subsystem, MIDI input, MIDI learn, or automation lanes. Existing transport/automation experiments or standalone primitives, if present on other branches, do not change the status of `main` and are not evidence of integration. This proposal defines a reviewable path; it does not claim these features are implemented.

In scope for design: internal transport, tempo map, metronome, MIDI input, MIDI learn, parameter mapping, automation playback/recording, schema-v2 persistence, and GUI integration.

Explicitly excluded from this proposal's v1 delivery: audio recording, plugin hosting, voice control, MIDI output/feedback, SysEx, MPE, power-loss durability guarantees, and external synchronization as a release requirement.

Binding constraints: offline build and runtime by default; no telemetry, call-home, update checks, remote configuration, or automatic network traffic; audio callbacks must not allocate, lock, log, access files, or use the network. GUI-originated changes must be represented as commands and pass through the operation log when undoable. New persistent data requires schema v2 while v1 projects remain readable.

## 2. Transport and tempo

### Clock source

Options:
- **Internal sample clock:** derives transport position from frames processed by the audio engine. Deterministic, portable, testable offline, and independent of external services.
- **External JACK/PipeWire transport or ALSA MIDI clock:** enables synchronization but adds backend-specific lifecycle, timestamp mapping, and testing requirements.

**Recommendation:** internal sample clock for v1. External sync is deferred to a separately approved design and optional backend.

### Accuracy target

The transport position and scheduled events should be sample-addressable. Commands arriving through the control queue are applied at an audio block boundary; therefore a GUI command has up to one block of command-to-effect scheduling delay, while the playhead's internal position advances by the exact number of frames processed. Events already queued for a block should carry a sample offset and be applied at that offset. If the implementation cannot honor sample offsets in the first increment, explicitly label it block-accurate, with worst-case timing error bounded by one block (e.g. 128 samples at a 128-frame period); do not describe that version as sample-accurate.

### Tempo map and schema

Recommend a tempo map rather than a single fixed tempo. Persist ordered points with this shape:

```json
"tempo_map": [
  {"position_beats": 0.0, "bpm": 120.0, "time_sig_num": 4, "time_sig_den": 4}
]
```

Points must have finite positive BPM, nonnegative strictly increasing beat positions, and valid time-signature values. Define whether a time-signature change is only allowed on a bar boundary; recommended: yes, validated by the tempo-map editor. Conversion must account for piecewise tempo changes and must not assume one constant BPM across the whole project.

Schema v2 adds the optional `transport` project settings and `tempo_map`. Schema-v1 reads must retain existing defaults and synthesize a default 120 BPM, 4/4 tempo map in memory without rewriting the source file until the user saves. The exact root shape is specified in section 6.

### Playhead state

Persist project-level defaults and edit state only (tempo map, loop-region definition when approved, click settings). Runtime-only state includes playing/paused/stopped, current sample position, pending seek, device clock, and callback counters. Runtime transport commands cross into the audio thread through a bounded SPSC queue of trivially copyable PODs. A full queue returns a visible non-blocking failure; never spin or silently claim a command was accepted.

### Metronome

Recommend click generation inside the audio processing path from precomputed click samples or a tiny preallocated oscillator/envelope, with no allocation in the callback. Pre-rendered click samples are simpler and deterministic; keep their buffers allocated before playback. Click enable and gain are queued parameter commands applied at block boundaries. Muting the click must not rebuild or mutate graph topology.

Open decisions: count-in behavior, loop-region persistence, whether transport is per project or app-global, and whether loop boundaries must be sample-accurate in v1.

## 3. MIDI input

### Backend comparison

| Backend | Advantages | Costs and implications |
|---|---|---|
| ALSA sequencer (`snd_seq`) | Native Linux MIDI support; fits the existing optional ALSA direction | Linux-specific; requires preinstalled ALSA development packages; device lifecycle and timestamp behavior need target testing |
| JACK MIDI | Common low-latency graph-based MIDI transport where JACK is installed | Adds optional dependency/runtime service and backend lifecycle; not universally available |
| PipeWire MIDI | Integrates with modern Linux desktop audio/MIDI routing | API and deployment vary by system; adds integration/test matrix requirements |

**Recommendation:** optional ALSA sequencer backend for the first Linux implementation, with a backend-neutral event interface. Keep it disabled unless explicitly configured and do not add dependency downloads. JACK/PipeWire are deferred backends; they are not silently substituted.

### Threading and event representation

A backend thread receives MIDI and writes a fixed-size event POD to a bounded single-producer/single-consumer ring. Illustrative shape:

```cpp
struct MidiEvent {
    std::uint64_t timestamp_samples;
    std::uint8_t status;
    std::uint8_t data1;
    std::uint8_t data2;
    std::uint8_t source_slot;
};
```

The event ring must have a documented overflow policy and counters; no dynamic allocation or blocking in the producer callback or audio consumer. The device thread must not call GUI widgets or mutate the project model.

### Timestamps and hotplug

Map backend timestamps to the engine sample-clock domain at receipt using a calibrated backend-to-engine offset. Late events are clamped to the earliest not-yet-processed sample and counted; future events remain queued up to a bounded horizon, after which they are rejected/counted according to an explicit policy. Do not invent precise timestamps when a backend only supplies coarse timing.

Device enumeration/rescan belongs on the GUI/control thread. Prefer explicit rescan initially; no background polling is required for v1. Disconnect handling must stop accepting events from the removed source and invalidate its learn mappings visibly, without dereferencing stale device objects.

Open decisions: multi-device merge order and clock arbitration. MPE, 14-bit CC, SysEx, MIDI output, and feedback are out of scope for v1.

## 4. MIDI learn and mapping

Proposed mapping record:

```json
{
  "source_device_id": "stable-local-id",
  "channel": 1,
  "message_type": "cc",
  "number": 7,
  "target_kind": "track_parameter",
  "target_id": "track-id",
  "param_id": "gain",
  "transform": {"min": 0.0, "max": 1.0, "curve": "linear"}
}
```

Use stable local device identifiers where available; do not persist transient enumeration indexes as identity. Validate channel, message type, controller number, target existence, and finite transform values.

Learn flow: user explicitly arms learn on a parameter; the next eligible event is captured on the MIDI/control thread and presented to the GUI; the user confirms or cancels; confirmation creates an operation-log entry and persists the mapping. The mapping is published to the audio path as an immutable snapshot or bounded command and becomes active at a documented block boundary. Never allocate mapping objects in the callback.

Automation mode proposal:
- **Read:** evaluate stored lane values while transport runs.
- **Touch:** while a control is actively touched, write/override; return to lane playback on release.
- **Latch:** after first touch, keep writing until transport stops or latch is explicitly disarmed.
- **Write:** continuously write the current controlled value while armed and transport runs.

Recommendation: propose Read for the first playback milestone; defer Touch/Latch/Write until recording buffers, undo grouping, and failure behavior have dedicated design and tests. MIDI learn itself is not a substitute for automation recording.

Latency target: MIDI receipt to parameter application within one audio block under normal load, with sample-offset scheduling used when backend timestamps and the event ring support it. This is a target to measure, not a current performance claim.

Open decisions: MIDI feedback/echo, takeover mode (jump/pickup/scale), conflict resolution for one source mapping to multiple targets, and whether duplicate mappings are permitted.

## 5. Automation lanes

Proposed lane identity is `(track_id, target_param_id)`. Each lane contains ordered breakpoints:

```json
{
  "track_id": "track-id",
  "target_param_id": "gain",
  "points": [
    {"position_samples": 0, "value": 1.0, "curve": "linear"}
  ]
}
```

Positions are nonnegative sample offsets from project start. Values are parameter-domain values validated against the target parameter's limits. v1 curve choices should be limited to `step` and `linear`; bezier/spline curves are deferred.

GUI edits create operation-log commands and update a control-thread project model. The audio thread reads an immutable, prevalidated automation snapshot. Publish snapshots through the repository's graph-publication/deferred-reclamation design only after that gate is accepted and proven safe; do not add an unprotected raw pointer swap. If safe reclamation is not ready, automation playback integration is blocked.

For record mode, the audio thread writes fixed-size breakpoint events into a preallocated SPSC record buffer. A non-audio thread drains it and batches points into one undoable recording-pass operation. On buffer overflow, increment an atomic counter and surface a visible incomplete-recording warning; never allocate or block in the callback. Define whether recording stops, drops newest points, or performs bounded thinning before implementation. Recommended initial policy: drop newest points with a visible warning; never silently claim a complete take.

Precedence recommendation: active manual touch overrides Read temporarily; on release, lane playback resumes. MIDI-mapped input is another parameter source and must use the same precedence arbiter rather than racing direct writes. The arbiter's ordering and priority need deterministic tests.

Open decisions: whether Write is included in v1, automation thinning/quantization, recording-pass undo granularity, and behavior when target tracks/parameters are deleted.

## 6. Schema v2 migration

Proposed JSON additions at project root:

```json
{
  "schema_version": 2,
  "transport": {
    "default_tempo_bpm": 120.0,
    "time_sig_num": 4,
    "time_sig_den": 4,
    "click_enabled": false,
    "click_gain": 0.25
  },
  "tempo_map": [
    {"position_beats": 0.0, "bpm": 120.0, "time_sig_num": 4, "time_sig_den": 4}
  ],
  "midi_mappings": [],
  "automation_lanes": []
}
```

The objects above are illustrative additions to the existing schema-v1 project shape; they do not replace `id`, `pan_law`, `tracks`, `buses`, or `routes`. The exact key names must be finalized before implementation.

Migration contract:
- Reader accepts schema 1 and schema 2.
- Schema 1 loads with defaults: internal clock, stopped transport, 120 BPM, 4/4, click disabled, no mappings, no automation lanes.
- Loading v1 does not mutate the source file. Saving a project that has been upgraded in memory writes schema 2.
- Schema 2 round-trips all supported fields and validates bounds/order.
- Unknown-field preservation is currently an open issue because the present reader is a strict subset parser rather than a general JSON DOM. Before promising preservation, either adopt a preinstalled JSON library with an offline dependency record or explicitly specify that unknown fields are rejected/ignored and lost on save. Recommendation: reject unsupported schema versions and document that v2 reader preservation of unknown fields is not guaranteed unless a parser capable of preserving them is selected.
- Never silently downgrade a v2 project to v1.

Required tests: v1 fixture load with unchanged track/bus/route values; v1 in-memory upgrade and v2 save; v2 full round-trip; malformed tempo and mapping rejection; unknown-field behavior matching the documented contract; operation-log undo/redo for mapping and lane edits.

## 7. Realtime contract impact analysis

| Subsystem | Audio-thread touchpoint | Allocation? | Lock? | I/O? | Required mitigation |
|---|---|---:|---:|---:|---|
| Transport | Consume fixed POD commands; advance sample counter | No | No | No | Fixed SPSC queue; atomics; prevalidated commands |
| Tempo map | Read immutable points / convert sample and beat positions | No | No | No | Precompute segments off-thread; bounded lookup |
| Click track | Mix preallocated click samples | No | No | No | Prepare buffers before playback; queued gain/enable |
| MIDI input | Consume fixed POD events | No | No | No | Backend thread + bounded ring; overflow counter |
| MIDI learn | No dynamic mapping edits in callback | No | No | No | Control-thread validation; immutable snapshot publication |
| Automation playback | Read immutable lane snapshot and interpolate | No | No | No | Safe graph/snapshot reclamation gate first |
| Automation record | Write fixed-size events to preallocated ring | No | No | No | Non-audio drain/batch; overflow surfaced |
| Persistence | No audio-thread file access | No | No | No | Serialize/parse only on control thread |

Any implementation path that introduces allocation, a blocking lock, logging, disk access, or network access in the audio callback is a release blocker. The source privacy audit must remain green. Its denylist should also cover newly introduced networking, telemetry, crash-upload, and update-check APIs in any new source directories and backend files. A green source audit is not proof of zero network activity.

## 8. Milestones and evidence gates

Every gate requires: automated tests or a machine-labelled report; README and `Docs/IMPLEMENTATION_STATUS.md` updates naming the evidence; a CHANGELOG entry; green `hush_privacy_source_audit`; default and relevant optional builds/tests green. Do not start a gate until dependencies and decisions for the preceding gate are accepted.

| Gate | Entry condition | Required evidence | Target-machine need | Claims unlocked only after pass |
|---|---|---|---|---|
| 1. Transport, tempo map, schema-v2 skeleton | Owner approves clock, accuracy, schema choices | State/seek/pause tests; tempo conversion and edge tests; v1→v2 and round-trip fixtures; sample scheduling error test | CI first; target audio device later | Transport/tempo implemented at the tested accuracy only |
| 2. Metronome/click | Gate 1 accepted | Deterministic render tests; click mute/gain queue tests; no-allocation callback test | Optional target audio check | Click track exists within tested limits |
| 3. MIDI backend/event ring | Backend choice approved; preinstalled dependency path documented | Virtual-device loopback; queue-overflow tests; timestamp mapping tests; latency report | Linux MIDI-capable target or virtual MIDI device | MIDI input for the tested backend only |
| 4. MIDI learn/persistence | Gate 3 accepted | Learn flow tests; mapping round-trip; invalid mapping tests; operation-log undo/redo | CI; hardware controller optional | MIDI learn for supported messages only |
| 5. Automation read playback | Safe snapshot reclamation gate complete | Step/linear interpolation; boundary and deleted-target tests; no-allocation callback test | CI; target audio verification later | Read-mode playback only |
| 6. Automation record and modes | Gate 5 accepted; owner approves modes/undo policy | Record-pass, overflow/failure, touch/latch/write, undo grouping tests | Target MIDI/audio device for latency report | Only the modes with passing evidence |
| 7. GUI integration | Relevant prior gates pass | GUI-to-command tests, queue-full UI indication, shortcuts/context menu tests, CLI/GUI parity | Headless Qt CI; optional human interaction check | Only GUI controls covered by tests |

The existing repository release gates for GUI wiring, graph reclamation, full routing, ALSA hardware validation, performance/soak, and autosave remain independently open. If the release-gate ledger changes, reconcile this table rather than silently changing gate order.

## 9. Risks and explicit non-claims

Top risks:
1. **Transport accuracy vs. GUI responsiveness:** control commands arrive asynchronously and block-boundary application adds latency. Mitigate with sample-addressable internal position and explicit sample offsets; test worst-case scheduling error.
2. **MIDI jitter on ALSA:** backend timestamps and host scheduling vary. Mitigate with timestamp-domain tests and machine-labelled latency reports; do not promise hard timing before measurement.
3. **Automation undo complexity:** a recording pass can generate many points and conflict with manual/MIDI control. Mitigate with batched undoable passes, bounded buffers, visible overflow, and an owner-approved precedence policy.

This proposal authorizes no claim of hardware validation, zero-network proof, soak stability, or a complete DAW. It does not authorize implementing recording, plugin hosting, voice control, SysEx, MPE, MIDI output, or power-loss durability. Those remain excluded unless separately proposed and approved.

## 10. Owner decision checklist

Implementation must not begin until the owner records decisions for:

1. Clock source: internal sample clock for v1? (recommended: yes)
2. Accuracy target: sample-addressable event scheduling, or block-accurate first release with a one-block maximum error?
3. MIDI backend: optional ALSA sequencer first, with a backend-neutral interface? (recommended: yes)
4. Automation v1: Read-only playback first, with Touch/Latch/Write deferred? (recommended: yes)
5. Schema v2 unknown fields: reject, ignore/loss on save, or require a preserving JSON parser?
6. Transport scope and loop/count-in: project-local or application-global; are loop and count-in in v1?
7. MIDI learn conflicts and manual-control precedence: which mapping/takeover policy should be used?
8. Automation recording undo granularity and buffer-overflow behavior.

**Owner approval:**  
- Decision owner: ____________________  
- Approved / rejected / revise: ____________________  
- Date: ____________________  
- Required changes before implementation: ____________________
