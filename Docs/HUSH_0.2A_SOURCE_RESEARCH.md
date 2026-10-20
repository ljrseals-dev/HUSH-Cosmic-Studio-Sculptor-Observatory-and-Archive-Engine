# HUSH 0.2a source research and implementation baseline

**Branch:** `milestone/0.2a-source-research`  
**Base:** `main` at the commit recorded in the branch history  
**Purpose:** isolate the 0.2 foundation audit and a small, testable contract correction from the release branch. This is a research/implementation branch, not a release claim.

## Source-derived baseline

The supplied HUSH milestone sequence defines 0.2 as a headless C++20 milestone covering schema-v1 persistence, invertible operation-log undo/redo, autosave and process-crash recovery, Linux ALSA with optional backends, preallocated processing, bounded parameter delivery, safe graph replacement, bus routing, meters/counters, and contract tests. GUI, recording, MIDI, plugins, voice control, and arrangement are explicitly excluded from 0.2.

The current repository source already contains project JSON round-trip, an operation log, recovery journal/autosave primitives, bus graph validation and offline rendering, a bounded SPSC queue, preallocated processing, realtime engine/host adapters, WAV read/write, and offline clip rendering. The README and release-gate ledger explicitly leave important evidence open.

## Change implemented in this branch

The mixer previously implemented the legacy -4.5 dB center pan law with a special case only when `abs(pan) < 1e-6`. Immediately to either side of center it used the unattenuated linear-balance formula, causing a discontinuity. This branch extracts a pure `pan_gains` helper, defines a continuous piecewise-linear legacy law, preserves the equal-power default, treats non-finite pan as center, clamps control pan to [-1, 1], and leaves float32 audio output unclamped. A dedicated contract test is registered in CTest.

## Remaining 0.2 gates — not certified by this branch

- 60-minute physical-device run at 48 kHz / 128 frames with zero xruns.
- Measured loopback latency and underrun-injection counter evidence.
- Eight-hour soak with memory/handle drift report.
- 500-track / 200-clip / 20-lane open-mutate-save-reopen performance gate.
- Complete track-to-bus-to-master live signal path, full mute/solo semantics, peak/RMS meters, and xrun counters.
- Fresh clean builds, CTest, ASan, and TSan on this branch head.
- Power-loss durability; current documented recovery evidence is process-restart and truncated-tail behavior.

These gates are not replaced or weakened. Hardware-dependent claims remain `not_run` until a named target machine and reproducible report exist.

## Validation plan

1. Configure offline with tests enabled.
2. Build all default targets.
3. Run CTest, including `hush_pan_law_tests`.
4. Repeat with ALSA enabled where system ALSA development files are installed.
5. Run ASan and TSan configurations; keep results tied to the exact commit SHA.
6. Keep hardware tests explicitly marked `not_run` unless actually executed on a physical device.
