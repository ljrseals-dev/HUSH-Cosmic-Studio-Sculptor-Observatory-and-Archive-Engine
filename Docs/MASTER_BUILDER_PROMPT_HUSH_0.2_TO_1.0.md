# MASTER BUILDER PROMPT — HUSH COSMIC STUDIO 0.2 → 1.0

**Target repository:** https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine  
**Target branch:** main  
**Language/build:** C++20, CMake, offline-first; optional Qt 6 Widgets and Linux ALSA when already installed.  
**Mission:** Finish the existing HUSH Cosmic Studio implementation in the exact ordered phases below. This is a production-engineering task, not a mockup, scaffold, or marketing exercise.

## Non-negotiable constraints

1. Preserve schema-v1 project compatibility and existing valid project files.
2. No telemetry, analytics, crash uploads, update checks, remote configuration, hidden automation, or runtime network requests.
3. No FetchContent, package bootstrap, curl/wget/Git clone, or network downloads in the normal build. Dependencies must already be installed.
4. The audio callback must not allocate, lock, log, perform file I/O, or destroy objects with nontrivial lifetime. Allocate all scratch buffers and construct graphs on a control thread before publishing them.
5. UI/control-to-audio parameter changes use bounded, preallocated SPSC queues. Apply complete parameter snapshots at block boundaries. Queue saturation must fail non-blockingly and be observable on the control side.
6. A graph replacement must not free an old graph while any audio callback can still reference it. Define and test the reader model and lifetime rules; do not use an unsafe raw-pointer swap.
7. Keep GUI/control, graph construction, persistence, and device lifecycle on control threads. The audio thread only consumes prepared state and processes buffers.
8. Do not claim a build, test, sanitizer, performance run, hardware check, latency measurement, zero-xrun result, or soak passed unless a reproducible command/report proves it.
9. Recording, MIDI, third-party plugin hosting, and voice control are out of scope unless the maintainer explicitly changes scope. Do not imply they exist.
10. Preserve committed archives until each archive has been inspected and its replacement/disposition is documented.

## Required ordered execution

### Phase 1 — Live mixer controls
- Connect Qt gain, pan, mute, solo, Undo, and Redo to the same canonical control state and the realtime engine.
- Enqueue changes through the bounded SPSC queue and apply them only at audio block boundaries.
- Make the GUI reflect queue rejection rather than silently claiming a command was applied.
- Add an automated Qt interaction/integration test proving UI action → command queue → audio-block state/output. If no real device is used in CI, test the callback with deterministic preallocated test buffers and label it as an integration test, not hardware playback.
- Document producer/consumer ownership and configure/start/stop/destruction ordering.

### Phase 2 — Safe graph publication
- Build replacement graph and processor state off the audio thread.
- Publish an immutable prepared snapshot using a lifetime protocol safe for the documented single-reader or multi-reader model.
- Reclaim retired graphs only on the control thread after proving no callback can access them.
- Exercise repeated publication and processing concurrently under ASan and TSan. Test shutdown and publication edge cases, including no active reader and an active reader.
- Integrate the publisher into the actual engine lifecycle; a standalone primitive test is not sufficient for phase completion.

### Phase 3 — End-to-end routing and mixer semantics
- Implement and validate track → bus → master routing, bus sends, inserts where present in the existing architecture, and documented pre-/post-fader taps.
- Define solo/mute interaction and test combinations across tracks and buses.
- Add peak/RMS metering and GUI-visible xrun counters using bounded/atomic communication; never log or update GUI widgets inside the callback.
- Add deterministic routing tests with known signals and expected outputs.
- If project track media/playback does not exist, explicitly identify that dependency and do not present the mixer shell as a full playback-capable DAW.

### Phase 4 — ALSA reliability evidence
- Harden startup/shutdown and device error handling; keep blocking device I/O outside the processing callback where the backend architecture requires it.
- Count xruns/recoveries atomically in the audio path and report them from the control thread.
- Keep ALSA-enabled configure/build/tests in CI.
- Provide runnable scripts for real-device startup/shutdown, loopback latency, underrun/recovery injection where practical, and a 60-minute stability run.
- Machine-dependent reports must include commit SHA, timestamp, OS/kernel, CPU, device/driver, sample rate, period size, duration, command line, xrun count, latency method/result, and status. Use status=not_run when no physical device was tested.

### Phase 5 — Performance and soak
- Make the routing benchmark configurable and provide a reproducible 500-track run with JSON output and a stated block-time budget.
- Run an eight-hour headless soak with parameter churn and recovery-journal activity.
- Track baseline, peak, and final memory; define an explicit drift threshold and pass/fail result rather than reporting RSS alone.
- Short CI smoke tests validate the harness and JSON schema only; they do not count as the 500-track performance run or eight-hour soak.

### Phase 6 — Recovery-journal guarantees
- Choose and document one honest guarantee:
  - **Process-crash recovery:** completed records survive process restart and partial/truncated trailing writes are safely ignored; or
  - **Power-loss durability:** implement and test required file and directory synchronization semantics and filesystem fault injection.
- Do not claim power-loss durability unless the latter evidence exists.
- Test truncated headers/payloads, interrupted writes, corrupted records, and preservation of the last complete snapshot.

### Phase 7 — Release cleanup and handoff
- Review every committed ZIP/archive before removal or relocation. Record its contents, source revision, and disposition.
- Verify .gitignore, dependency/license inventory, privacy/security notes, changelog, roadmap, README, and release-gate status agree with actual source and evidence.
- Run clean offline builds and CTest with ALSA disabled and enabled, GUI enabled where Qt is installed, plus ASan and TSan configurations.
- Produce a source ZIP only after generating it from a clean, identified commit; inspect the ZIP contents and verify its SHA-256. Publish the link only after the artifact exists and has been verified.
- Do not call the project 1.0 complete while required phases or hardware reports remain open.

## Required engineering workflow

For each phase:
1. Inspect existing code and tests before changing APIs.
2. Implement the smallest complete slice; do not duplicate canonical project state.
3. Add tests with the change and register them in CMake/CTest.
4. Build and run the relevant tests locally or in CI. Fix failures before proceeding.
5. Update Docs/PHASE_1_TO_7_EXECUTION.md, Docs/RELEASE_GATES.md, ROADMAP.md, and CHANGELOG.md to match evidence.
6. Commit to a named branch, open a PR against main, and link the successful CI run.
7. Merge only when CI passes and remaining gaps are explicit. A merge does not convert unimplemented work into completed work.

## Current known state at prompt creation

The repository already has a schema-v1 project model, operation-log undo/redo primitives, bus graph and offline renderer, preallocated bus processing, bounded SPSC queue, realtime bus engine/adapter, optional ALSA host, optional Qt mixer shell, a realtime track-mixer primitive, a graph-publication primitive, recovery-journal tests, benchmark/soak harnesses, and CI smoke checks.

Known gaps that must be closed rather than hidden:
- Qt controls/Undo/Redo are not yet bound to a running live engine callback.
- Graph publication is not yet integrated with the production engine lifecycle.
- Complete track → bus → master routing, inserts, end-to-end mute/solo, meters, and GUI xrun display are not complete.
- No named-device hardware report, loopback latency report, or 60-minute zero-xrun report is attached.
- No qualifying 500-track performance report or eight-hour soak report is attached; memory drift has no enforced threshold.
- Power-loss durability is not claimed.
- Existing ZIP artifacts still require content review and release disposition.
- The existing Qt shell edits project state but must not be described as live playback until the actual engine/device path is connected and tested.

## Definition of done

HUSH Cosmic Studio 1.0 is complete only when every in-scope feature is implemented and tested, every machine-dependent claim has a reproducible evidence report, privacy/offline constraints are verified, release artifacts are built and checksummed, and all release gates pass. If physical hardware or a long-duration run is unavailable, keep that gate explicitly open and provide the exact runnable command/report schema—never fabricate the result.
