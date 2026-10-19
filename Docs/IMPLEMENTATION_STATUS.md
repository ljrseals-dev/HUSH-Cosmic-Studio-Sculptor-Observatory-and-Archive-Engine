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
- [x] GUI, recording, MIDI, plugins, and voice remain out of scope.

## Remaining release gates
- [ ] Integrate the autosave controller into a running application lifecycle and surface save failures in the host UI/log. The current repository has no running app/transport lifecycle.
- [ ] Validate kill/restart behavior on supported target filesystems and verify directory-entry durability where required; the current subprocess test covers completed writes, not power loss or mid-write termination.
- [ ] ALSA real-time backend and optional JACK/PipeWire compile-time backends.
- [ ] Preallocated processing graph and bounded lock-free parameter queue.
- [ ] Graph swap with documented and implemented safe reclamation.
- [ ] Integrate the reference bus renderer into a preallocated real-time processing graph; the current renderer allocates buffers and is not audio-callback safe.
- [ ] Inserts and project-track-to-bus assignment.
- [ ] Meter/xrun readouts connected to a real device backend.
- [ ] 60-minute 48 kHz/128-frame zero-xrun reference run.
- [ ] Loopback latency measurement and underrun injection.
- [ ] Nested-bus render byte-identical to flat-bus render test.
- [ ] 8-hour soak and 500-track/200-clip/20-lane performance gate.
- [ ] Preserve all 0.1.0 tests (not available in this repository's current source archive).

Do not mark 0.2 release-ready until all required gates have evidence. A passing unit-test job does not prove real-time audio behavior.
