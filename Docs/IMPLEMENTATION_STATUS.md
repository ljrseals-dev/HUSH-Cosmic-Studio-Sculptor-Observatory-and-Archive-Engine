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
- [x] Interleaved-stereo adapter deinterleaves device input, injects it into a configured graph input bus, runs RealtimeBusEngine, and interleaves the configured master bus output; scratch is preallocated and offline contract tests are added.
- [x] Optional Linux ALSA capture/playback worker target and callback contract (device validation pending).
- [x] GUI, recording, MIDI, plugins, and voice remain out of scope.

## Remaining release gates
- [ ] Integrate the autosave controller into a running application lifecycle and surface save failures in the host UI/log. The current repository has no running app/transport lifecycle.
- [ ] Validate kill/restart behavior on supported target filesystems and verify directory-entry durability where required; the current subprocess test covers completed writes, not power loss or mid-write termination.
- [ ] Wire a running host/device lifecycle that constructs BusGraph, configures RealtimeBusEngine and RealtimeBusAudioAdapter, and starts/stops AlsaAudioBackend; the reusable callback adapter is implemented, but no app host currently owns that lifecycle.
- [ ] Optional JACK/PipeWire compile-time backends.
- [ ] Verify parameter atomics are lock-free on target hardware and test ALSA device behavior.
- [ ] Safe graph swap and deferred reclamation; current processor is configure-once and must not be destroyed while processing.
- [ ] Integrate track-to-bus assignment, inserts, solo/mute semantics, meters, and xrun counters into the processing path.
- [ ] 60-minute 48 kHz/128-frame zero-xrun reference run.
- [ ] Loopback latency measurement and underrun injection.
- [ ] Nested-bus render byte-identical to flat-bus render test.
- [ ] 8-hour soak and 500-track/200-clip/20-lane performance gate.
- [ ] Preserve all 0.1.0 tests (not available in this repository's current source archive).

Do not mark 0.2 release-ready until all required gates have evidence. A passing unit-test job does not prove real-time audio behavior.
