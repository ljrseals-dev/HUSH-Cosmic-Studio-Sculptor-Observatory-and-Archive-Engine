# HUSH Cosmic Studio roadmap

This roadmap follows the ordered gates in [Docs/Prompt](Docs/Prompt). A milestone is not complete merely because its source compiles; the listed tests and target-machine evidence are required.

## Current state — 0.3a GUI foundation

- [x] Headless C++20 project model, schema-v1 persistence, bus graph validation, offline renderer, preallocated bus processor, SPSC fader queue.
- [x] Optional ALSA host and CI compile/test job.
- [x] Optional Qt 6 mixer shell and CI compile/test job.
- [ ] GUI controls bound to live audio via a block-boundary parameter command path.
- [ ] GUI test for queued command → engine application; GUI/CLI project parity.
- [ ] Safe graph replacement and deferred reclamation.
- [ ] End-to-end track → bus → master routing, inserts, mute/solo, meters and xrun readouts.
- [ ] Target-machine ALSA startup/shutdown, xrun, loopback latency and 60-minute validation evidence.
- [ ] 500-track performance report and 8-hour soak report.
- [ ] Automated sanitizer CI and hardware-aware release evidence.

## Ordered next gates

1. **0.3a integration:** live parameter bridge with a single-producer/single-consumer ownership contract; do not mutate graph topology from GUI/audio callback.
2. **Graph lifetime:** select and document an epoch/deferred-reclamation design before implementing graph swaps; test under ASan/TSan.
3. **Routing and observability:** track/bus/master semantics, meter snapshots and xrun counters, all read without callback locks.
4. **Host validation:** runnable scripts for loopback latency and zero-xrun duration; report JSON and label hardware/environment.
5. **Scale/soak:** configurable performance budget, long-duration host soak, memory drift checks and JSON reports.
6. **Durability:** current claim is process-crash/restart recovery only, not a power-loss guarantee.
7. **Release hygiene:** preserve dependency/offline constraints, keep CI green, document each completed gate with evidence.

## Out of scope until later milestones

Recording, MIDI, plugin hosting, Windows/macOS parity, session interchange and HUSH voice/AOC Magic are not part of the current 0.3a gate. See [Docs/Prompt](Docs/Prompt) for the full sequence.
