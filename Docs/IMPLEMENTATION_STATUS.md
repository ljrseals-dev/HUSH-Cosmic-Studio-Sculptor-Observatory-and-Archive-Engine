# HUSH 0.2 implementation status

## In this branch
- [x] C++20 offline CMake build definition.
- [x] Schema-v1 project model and JSON round-trip subset.
- [x] Structured error contract.
- [x] Invertible operation-log primitives.
- [x] Explicit pan law options.
- [x] Float32 mixing primitives and solo/mute semantics.
- [x] Contract tests for implemented primitives.
- [x] GUI, recording, MIDI, plugins, and voice remain out of scope.

## Remaining release gates
- [ ] Autosave timer and transport-stop autosave.
- [ ] Durable journal + kill/restart recovery test.
- [ ] ALSA real-time backend and optional JACK/PipeWire compile-time backends.
- [ ] Preallocated processing graph and bounded lock-free parameter queue.
- [ ] Graph swap with documented and implemented safe reclamation.
- [ ] Bus routing, sends/inserts, and hard cycle detection.
- [ ] Meter/xrun readouts connected to real device backend.
- [ ] 60-minute 48 kHz/128-frame zero-xrun reference run.
- [ ] Loopback latency measurement and underrun injection.
- [ ] Nested-bus vs flat-bus byte-identical render test.
- [ ] 8-hour soak and 500-track/200-clip/20-lane performance gate.
- [ ] Preserve all 0.1.0 tests (not available in this repository's current source archive).

Do not mark 0.2 release-ready until all required gates have evidence. A passing unit-test job does not prove real-time audio behavior.
