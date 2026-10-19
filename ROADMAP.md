# HUSH Cosmic Studio roadmap

This roadmap tracks the 0.2 foundation toward a release. A milestone is not complete merely because its source compiles; required tests and target-machine evidence must be recorded.

## Current baseline — 0.2.0 / optional 0.3a GUI shell

- [x] Headless C++20 project model, schema-v1 persistence, bus graph validation, offline renderer, preallocated bus processor, SPSC parameter queue.
- [x] Optional ALSA backend/host and CI configuration.
- [x] Optional Qt 6 mixer shell and CI configuration.
- [x] Recovery journal process-restart test and truncated-tail handling.
- [ ] Verify remaining committed archive contents and remove redundant binary archives.
- [ ] Wire GUI controls to live audio; add queued-command interaction tests and GUI/CLI parity.
- [ ] Implement safe graph replacement and deferred reclamation.
- [ ] Complete track-to-bus/master routing, insert placement, end-to-end mute/solo, meters, and xrun counters.
- [ ] Validate ALSA startup/shutdown, callback timing, atomic lock-free suitability, and xrun reporting on target hardware.
- [ ] Commit 60-minute zero-xrun, loopback-latency, and underrun-injection evidence.
- [ ] Run `hush_routing_benchmark 500 <seconds>` and commit its JSON output with a documented real-time budget threshold.\n- [ ] Run `hush_headless_soak 28800` and commit its JSON output; define and enforce a memory-drift threshold.
- [ ] Integrate autosave with application lifecycle and surface save failures.
- [x] Add partial-header and truncated-payload recovery fault tests (test execution still needs CI evidence).\n- [ ] Establish durability claims with power-loss tests before making them; until then, only process-restart recovery is claimed.

## Ordered gates

1. **GUI wiring:** connect supported controls to a bounded block-boundary command path without mutating graph topology from the GUI or audio callback.
2. **Graph lifetime:** choose/document deferred-free, hazard-pointer, or epoch reclamation; stress under sanitizers.
3. **Full routing:** define and test track/bus/master semantics, inserts, mute/solo, meters, and xrun counters.
4. **ALSA validation:** target-machine startup/shutdown, callback timing, loopback latency, underrun injection, and sustained zero-xrun evidence.
5. **Performance and soak:** reproducible 500-track budget test and 8-hour soak with memory-growth report.
6. **Journal durability:** test failure modes and power-loss behavior before expanding guarantees.
7. **Release hygiene:** offline dependency policy, complete archive audit, green CI, traceable README claims, and documented evidence.

## Out of scope for the 0.2 foundation

Recording, MIDI, plugin hosting, Windows/macOS parity, arrangement view, interchange, and HUSH voice/AOC Magic remain later milestones. See [Docs/Prompt](Docs/Prompt) for the full prompt sequence.
