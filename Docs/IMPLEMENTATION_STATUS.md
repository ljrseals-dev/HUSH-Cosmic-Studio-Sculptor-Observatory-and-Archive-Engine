# HUSH 0.2 implementation status
## Implemented in this branch
- [x] C++20 offline CMake build definition.
- [x] Schema-v1 project model and JSON round-trip for tracks, buses, and routes.
- [x] Backward-compatible reading of older schema-v1 files without bus/route arrays.
- [x] Persisted bus-routing validation at the project-load boundary (unknown endpoints, duplicate/empty bus IDs, self-routes, invalid gain, and cycles).
- [x] Append-only recovery journal with length-framed full-project snapshots, stable-storage sync, and recovery that ignores an incomplete trailing record.
- [x] Autosave scheduling policy for first poll, periodic snapshots, and transport-stop snapshots; caller must poll from a non-audio thread.
- [x] Invertible operation-log primitives.
- [x] Explicit pan law options.
- [x] Float32 mixing primitives and solo/mute semantics.
- [x] Contract tests for implemented primitives, routing persistence, cycle rejection, and torn-tail recovery.
- [x] Child-process exit/restart test for recovering a completed journal snapshot.
- [x] Bus graph validation, send tap metadata, and source-before-destination render order.
- [x] Offline/reference bus-buffer renderer with pre/post-fader sends, deterministic topological processing, and input validation.
- [x] Fixed-capacity single-producer/single-consumer parameter queue primitive and deterministic contract tests.
- [x] Preallocated bus processor with control-thread setup, fixed-capacity buffers, indexed routes, and an allocation-free process path.
- [x] RealtimeBusEngine composes the preallocated processor with the bounded SPSC fader-parameter queue; queue updates are applied at processing-block boundaries.
- [x] Interleaved-stereo adapter injects device input into a selected graph input bus, runs RealtimeBusEngine, and emits the selected master's post-fader output; scratch is preallocated and offline contract tests pass in CI.
- [x] Optional Linux ALSA capture/playback worker target and callback contract.
- [x] Optional Linux host executable constructs a capture-to-master graph, configures engine and adapter, starts ALSA, and stops cleanly on SIGINT/SIGTERM.
- [x] Optional Qt 6 Widgets mixer GUI shell with schema-v1 open/save, gain/pan/mute/solo controls, and undo/redo through the operation log.
- [ ] GUI meter polling, send/insert controls, CLI parity test, live-engine binding, and GUI session soak remain incomplete.
- [x] Recording, MIDI, plugins, and voice remain out of scope for this milestone.

## Remaining release gates
- [ ] Build the ALSA-enabled configuration on a Linux host with ALSA development headers/libraries; the default CI job builds/tests with ALSA disabled.
- [ ] Test device startup/shutdown on named ALSA hardware, verify parameter atomics are lock-free on target hardware, and measure callback timing/xruns.
- [ ] Integrate the autosave controller into a running application lifecycle and surface save failures in a host UI/log.
- [ ] Validate kill/restart behavior on supported target filesystems and verify directory-entry durability where required; current subprocess test covers completed writes, not power loss or mid-write termination.
- [ ] Optional JACK/PipeWire compile-time backends.
- [ ] Safe graph swap and deferred reclamation; current processor is configure-once and must not be destroyed while processing.
- [ ] Integrate track-to-bus assignment, inserts, solo/mute semantics, meters, and xrun counters into the processing path.
- [ ] 60-minute 48 kHz/128-frame zero-xrun reference run.
- [ ] Loopback latency measurement and underrun injection.
- [ ] Nested-bus render byte-identical to flat-bus render test.
- [ ] 8-hour soak and 500-track/200-clip/20-lane performance gate.
- [ ] Preserve all 0.1.0 tests (not available in this repository's current source archive).

Do not mark 0.2 release-ready until all required gates have evidence. A passing default CI job does not prove real-time audio behavior or compile the optional ALSA target.
