# Routing gate audit — Gate 3

Status: **AUDIT COMPLETE; IMPLEMENTATION OPEN**  
Branch: `docs/routing-gate-audit-20261019`  
Scope: source review only. This document does not claim routing, meter, insert, or hardware behavior beyond the code and tests already present.

## Current graph shape

- `Project` persists tracks (`id/name/gain/pan/mute/solo`) separately from buses and `BusRoute` entries. The persisted route endpoints are bus IDs only.
- `BusGraph::configure` validates unique/non-empty bus IDs, known route endpoints, self-routes, finite gain in [0, 16], and directed cycles. The validator is control/setup-side and allocates.
- `PreallocatedBusProcessor::configure` allocates per-bus pre- and post-fader stereo buffers and resolves bus-to-bus routes and render order. Its `process()` is documented as allocation-, lock-, log-, and I/O-free.
- `RealtimeBusEngine` uses a bounded SPSC queue for bus fader changes and applies them while processing blocks. It is configure-once; topology changes require a replacement engine.
- `RealtimeBusAudioAdapter` bridges one interleaved stereo input into a configured engine's selected input bus and copies the selected master bus to output.
- `alsa_realtime_host.cpp` currently builds a two-bus `capture -> master` graph and runs the bus-engine host adapter. The separate Qt live mixer path uses `RealtimeTrackMixer`, with capture L/R mapped to track indexes 0 and 1. These are two distinct paths, not a unified track/bus/master graph.

## Current mute/solo behavior and precedence

- Track gain/pan/mute/solo are represented in the schema-v1 `Track` model and handled by `RealtimeTrackMixer` on its own path.
- Bus engine state currently exposes fader updates, not end-to-end track mute/solo.
- The current code does not establish a single end-to-end solo/mute precedence rule across tracks, buses, sends, and master. Gate 3 must define and test one before claiming completion. This audit does not silently infer behavior from the separate track mixer.
- Proposed acceptance rule for owner review: **solo is evaluated first as an in-place eligibility mask; muted tracks remain silent even if soloed; when any track is soloed, only soloed and non-muted tracks contribute; master mute silences the final output.** This is a proposed design decision, not yet implemented or test-backed.

## Gaps

1. No persisted track-to-bus membership or track-to-bus route endpoint exists.
2. No unified live graph connects source tracks, track processing, buses, sends, and master.
3. No fixed-capacity insert chain exists on each track/bus. Built-in DSP insert capacity and bypass semantics need explicit contract/tests before implementation.
4. Pre/post-fader bus sends exist as bus-to-bus route metadata. Track-to-bus sends and a shared routing path do not.
5. No per-node peak/RMS snapshot is exposed to the GUI.
6. ALSA recovery/short-transfer counters exist as backend diagnostics, but the audit has not established a distinct, exhaustively classified `-EPIPE` xrun counter or stubbed write-failure test.
7. The Qt live monitor has callback/recovery status polling, but no peak/RMS meters or full routing selector/insert UI.
8. Current tests cover bus validation/processing and track mixer behavior separately; no N×M track-to-bus routing matrix, unified mute/solo semantics, insert-chain, or meter tolerance test exists.

## Meter and xrun attachment points

- Meter calculation belongs in preallocated per-node processing buffers, with publication via a bounded/lock-free snapshot that is read only off the audio thread. No GUI polling may inspect mutable audio-thread-owned buffers directly.
- Xrun classification belongs at ALSA read/write/recovery boundaries in `src/alsa_audio_backend.cpp`; publish atomic counters to the host/GUI, and summarize only after the backend has stopped. Do not log or perform I/O in the callback.
- Exact lock-free guarantees for the chosen atomics must be checked on the target platform; compile success alone is insufficient.

## Schema impact and compatibility

- Existing schema v1 stores bus-to-bus routes only and must continue loading unchanged.
- Persisted track-to-bus membership, track sends, or persisted insert configuration are new fields and require a schema v2 bump, a backward-compatible v1 reader with defined defaults, and v1/v2 round-trip tests. Do not smuggle these fields into schema v1.
- Runtime-only meters, xrun counters, and diagnostic snapshots should not be persisted.
- Cycle validation must be applied on load and to every structural-edit candidate before publication. Structural edits must publish a fully constructed replacement graph off the audio thread and defer reclamation until the reader can no longer hold the old graph.

## Existing test coverage observed

- `tests/bus_graph_tests.cpp`: bus graph validation and cycle handling.
- `tests/preallocated_bus_processor_tests.cpp`: preallocated bus processing.
- `tests/realtime_bus_engine_tests.cpp`: queued bus fader/block processing.
- `tests/realtime_bus_audio_adapter_tests.cpp`: interleaved stereo adapter.
- `tests/realtime_track_mixer_tests.cpp`: separate track gain/pan/mute/solo path.
- `tests/realtime_graph_publisher_tests.cpp` and `tests/published_realtime_bus_engine_tests.cpp`: publication primitive/wrapper tests; the wrapper is not yet integrated into the GUI/ALSA host.
- These are source/test inventory findings; they do not establish target-hardware performance.

## Gate 3 exit criteria

- [ ] Select and document solo/mute precedence in the implementation contract.
- [ ] Define insert capacity and behavior for full/invalid chains.
- [ ] Add schema v2 and backward-compatible v1 tests before persisting new route/insert fields.
- [ ] Add track-to-bus-to-master routing matrix tests, cycle rejection on structural edit, and live graph-swap stress.
- [ ] Add deterministic insert passthrough/modification/capacity tests.
- [ ] Add per-node peak/RMS tolerance tests and a safe snapshot publication test.
- [ ] Add ALSA stub failure injection for `-EPIPE` and other write outcomes; assert counters and recovery behavior.
- [ ] Wire GUI/CLI only to implemented shared semantics; record parity gaps explicitly.
- [ ] Run fresh default, ALSA, GUI+ALSA, ASan, and TSan CI after implementation.
- [ ] Do not claim hardware, 60-minute zero-xrun, loopback, 500-track qualifying performance, or eight-hour soak without machine-labelled reports.

## Realtime contract

All topology validation, allocation, insert construction, and graph replacement construction occur on the control thread. The callback may only read a published immutable topology, consume bounded preallocated commands, process into preallocated buffers, update bounded lock-free snapshots/counters, and return. If a required feature cannot meet this contract, log the blocker rather than weakening it.
