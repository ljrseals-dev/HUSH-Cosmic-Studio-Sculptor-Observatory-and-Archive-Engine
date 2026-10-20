# HUSH Cosmic Studio roadmap

This roadmap tracks the 0.2 foundation toward a release. A milestone is not complete merely because its source compiles; required tests and target-machine evidence must be recorded.

## Current baseline — 0.2.0 / optional 0.3a GUI shell

- [x] Headless C++20 project model, schema-v1 persistence, bus graph validation, offline renderer, preallocated bus processor, SPSC parameter queue.
- [x] Optional ALSA backend/host and CI configuration.
- [x] Optional Qt 6 mixer shell and CI configuration; combined Qt + ALSA live-monitor build added to CI.
- [x] Recovery journal process-restart test and truncated-tail handling.
- [x] Validate the project bus-to-WAV export path in CI (PR #42); complete-arrangement rendering remains separate.
- [x] Add offline PCM16/float32 WAV decoding with size limits and contract tests.
- [x] Add optional schema-v1 media-source metadata and sample-positioned clip persistence with validation tests.
- [ ] Connect decoded clip buffers to track gain/pan/mute/solo and route complete arrangement rendering through the bus graph.
- [ ] Verify remaining committed archive contents and remove redundant binary archives.
- [x] Wire GUI fader/pan/mute/solo and undo/redo to bounded realtime commands through an explicitly started ALSA live monitor (source mapping limited to capture L/R → tracks 1–2).
- [ ] Add Qt interaction/loopback tests and GUI/CLI parity; run combined Qt + ALSA CI and target-device checks.
- [ ] Implement safe graph replacement and deferred reclamation.
- [ ] Complete track-to-bus/master routing, insert placement, end-to-end mute/solo, meters, and xrun counters.
- [ ] Validate ALSA startup/shutdown, callback timing, atomic lock-free suitability, and xrun reporting on target hardware.
- [ ] Commit 60-minute zero-xrun, loopback-latency, and underrun-injection evidence.
- [x] Add configurable p99 block-time budget, sampled timing statistics, explicit pass/fail JSON, and a 500-track benchmark smoke test (full-duration qualification remains open).
- [x] Add configurable positive RSS-drift threshold and a bounded-memory soak smoke test (8-hour run/report remains open).
- [ ] Integrate autosave with application lifecycle and surface save failures.
- [x] Add partial-header and truncated-payload recovery fault tests (test execution still needs CI evidence).
- [ ] Establish durability claims with power-loss tests before making them; until then, only process-restart recovery is claimed.

## Ordered gates

1. **GUI wiring:** source integration is in place for a user-started ALSA monitor and bounded block-boundary track controls; remaining acceptance gates are combined CI, Qt interaction/loopback tests, and target-device evidence.
2. **Graph lifetime:** choose/document deferred-free, hazard-pointer, or epoch reclamation; stress under sanitizers.
3. **Full routing:** define and test track/bus/master semantics, inserts, mute/solo, meters, and xrun counters.
4. **ALSA validation:** target-machine startup/shutdown, callback timing, loopback latency, underrun injection, and sustained zero-xrun evidence.
5. **Performance and soak:** reproducible 500-track budget test and 8-hour soak with memory-growth report.
6. **Journal durability:** test failure modes and power-loss behavior before expanding guarantees.
7. **Release hygiene:** offline dependency policy, complete archive audit, green CI, traceable README claims, and documented evidence.

## Out of scope for the 0.2 foundation

Recording, MIDI, plugin hosting, Windows/macOS parity, arrangement view, interchange, and HUSH voice/AOC Magic remain later milestones. See [Docs/Prompt](Docs/Prompt) for the full prompt sequence.


## HUSH 0.4a recording foundation

- [x] Add a preallocated SPSC capture ring with bounded, non-blocking producer behavior and explicit dropped-frame counts (PR #75; CI pending).
- [ ] Add a disk-consumer worker and streaming WAV finalization; file I/O must remain off the audio callback.
- [ ] Integrate capture source/track arming, take IDs and playlists, then punch/comping/crossfades and latency compensation.
- [ ] Run the Prompt 4 recording gates on deterministic test input and a named audio device; hardware results remain not_run until measured.
