# HUSH Cosmic Studio 0.2 → 1.0 — Phase 1–7 execution record

This file is an evidence ledger, not a claim that unfinished work is complete. A phase moves to **PASS** only when its implementation, automated test evidence, and (where applicable) target-machine report are committed.

## Global release invariants

- Zero telemetry and no runtime/build-time network dependency.
- No FetchContent or dependency bootstrap in the build.
- Audio callback code must not allocate, lock, log, or perform file I/O.
- Control-to-audio changes cross a bounded, preallocated SPSC queue and are applied at block boundaries.
- Preserve schema-v1 project compatibility.
- Do not describe an unrun hardware, latency, performance, or soak test as passing.

## Phase status

| Phase | Required implementation and evidence | Status at this revision |
|---|---|---|
| 1. Live mixer control | Connect the Qt mixer to the live engine; queue fader, pan, mute and solo changes; apply at block boundaries; Qt test proves enqueue and application; document thread ownership in `Docs/HUSH_0.3A_MIXER_GUI.md`. | **PARTIAL** — GUI controls and undo/redo enqueue commands into `RealtimeTrackMixer` for an explicitly started optional ALSA monitor. Startup seeds all track controls; subsequent edits and undo/redo enqueue only the affected track. A headless Qt widget test has been added in `tests/gui_interaction_tests.cpp`; CI for this branch is pending. Prior run 39255426377 passed seven jobs on an earlier revision, not this test change. Capture L/R currently map to tracks 1–2 only. Exact widget-to-queue assertion, loopback and target-device evidence remain open. |
| 2. Safe graph swap | Construct replacement graph off the audio thread; publish safely; defer destruction until no callback can reference the old graph; stress-test under audio load with ASan and TSan. | **PARTIAL** — `PublishedRealtimeBusEngine` wraps `RealtimeGraphPublisher`, configures replacement engines on the control thread, publishes them for callback processing, holds a read guard for each process call, and defers old-engine reclamation. Held-reader and concurrent replacement tests are registered. The wrapper is not yet connected to the GUI or ALSA host; CI for the latest revision is pending. |
| 3. Track → bus → master | Implement end-to-end routing, inserts, correct solo/mute behavior, peak/RMS meters and GUI-visible xrun counters; add routing/semantics tests. | **OPEN** — bus primitives exist, but complete track graph and GUI meter semantics are not present. |
| 4. ALSA hardening | Handle/report `-EPIPE` xruns outside the callback; run hardware startup/shutdown, loopback latency and 60-minute stability tests; attach target-machine JSON; retain ALSA-enabled CI. | **PARTIAL** — ALSA-enabled CI compiles/tests the backend; no target-hardware report is attached. |
| 5. Performance and soak | Add a configurable 500-track routing benchmark and an 8-hour headless synthetic-traffic soak with journal/parameter churn, memory-drift threshold and JSON output. | **PARTIAL** — added `hush_routing_benchmark` and `hush_headless_soak` tools with JSON output, configurable duration, parameter churn and journal snapshots. CI smoke tests pass, but the required 500-track benchmark and 8-hour run have not yet been executed; RSS drift is reported but no pass threshold is configured. |
| 6. Journal durability | Either prove fsync-based power-loss durability with directory metadata handling and fault injection, or explicitly scope the guarantee to process-crash recovery and test partial/truncated writes. | **PARTIAL** — power-loss durability is not claimed; completed-record restart and truncated-tail behavior are covered. Partial-write test cases are registered and passed in CI; power-loss durability is not claimed. |
| 7. Cleanup/release readiness | Remove or relocate committed ZIP artifacts only after their contents are reviewed; add ignore rules and changelog/roadmap; align claims with tests; verify ALSA-off/on, GUI, ASan and TSan CI. | **PARTIAL** — CI previously passed Linux C++20, ALSA-enabled, Qt6 GUI, ASan and TSan jobs. The successful CI run also built and smoke-tested the benchmark/soak tools and validated their JSON output; qualifying 500-track and eight-hour reports remain outstanding. Archive disposition, ignore/changelog/roadmap review, and final evidence reconciliation remain open. |

## Required report format for machine-dependent tests

Store each report as JSON and include: tested commit SHA, timestamp, OS/kernel, CPU, audio interface and driver, sample rate, period/buffer size, duration, command line, xrun count, latency method and measured result, memory baseline/peak/end, threshold, and pass/fail. Use `"status": "not_run"` for a test that was not executed. Do not fabricate hardware details or measurements.

## Completion conclusion

**HUSH Cosmic Studio 0.2 → 1.0 is not yet release-complete.** Phase 1 now has a source-level Qt-to-engine command path for the optional ALSA monitor, but still lacks Qt interaction/loopback tests and full track-source routing; Phase 2 now has a bus-engine publication wrapper and concurrent contract test, but still needs fresh CI and host/application integration; Phase 3 has implementation gaps; Phase 5 now has harnesses but lacks qualifying run evidence; Phase 4 requires target hardware evidence; Phase 6 needs explicit partial-write fault-injection evidence or a clearly documented narrower guarantee; Phase 7 requires final artifact and CI reconciliation. This record intentionally does not turn a successful compile into a claim of realtime stability, low latency, zero xruns, or long-duration reliability.

Recording, MIDI, plugin hosting, and voice control remain out of scope unless separately specified.
