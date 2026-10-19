# HUSH Master Builder Prompt — Open All Implementation Gates

**Project:** HUSH Cosmic Studio — Sculptor, Observatory, Archive & Engine  
**Language/build:** C++20, CMake, native Qt 6 Widgets GUI (optional, preinstalled dependency only)  
**Execution mode:** Authorized to proceed through all implementation phases without asking for repeated approval at each phase.

## Mission

Act as the lead C++ audio-engine and DAW integration engineer. Inspect the current repository and implement the highest-value complete, testable vertical slices. Do not stop at plans, mockups, scaffolding, or status-only changes when source implementation is possible. Work directly from existing APIs and preserve compatible project formats.

## Open-gate authorization

Proceed through all engineering phases in sequence without waiting for another confirmation:

1. Inspect repository, build configuration, tests, implementation-status ledger, release-gate ledger, and privacy contract.
2. Connect the Qt mixer controls to a real control-to-audio command path. Implement gain/pan/mute/solo only where engine semantics can be made correct and testable; never pretend a project-model edit changed live audio.
3. Add deterministic tests for queue-full/invalid commands, block-boundary application, GUI-to-engine command behavior where practical, and model/engine consistency.
4. Implement track-to-bus/master processing and routing only with explicit, validated graph semantics. Preserve the audio callback constraints.
5. Design and implement safe graph publication/replacement and deferred reclamation with race-focused tests; do not free or mutate audio-thread-owned graph memory from the control thread.
6. Add honest meters, xrun/error counters, backend lifecycle/error reporting, and target-device test harnesses where supported.
7. Improve recovery/autosave lifecycle and surface save failures to the user.
8. Run available offline configure/build/tests and sanitizer jobs; update CI and docs to match the actual evidence.
9. Update `Docs/IMPLEMENTATION_STATUS.md`, `Docs/RELEASE_GATES.md`, `ROADMAP.md`, and README with implemented behavior, test commands, known limits, and evidence.
10. Deliver a concise handoff with exact commit/PR links and a list of tests actually run. If an archive is produced, verify that the file exists and contains the expected sources before linking it.

## Non-negotiable architecture and privacy contract

- C++20; no Rust source, runtime, FFI, Cargo, or Rust UI bridge.
- No telemetry, analytics, crash uploads, automatic update checks, remote configuration, license pings, webhooks, call-home behavior, or unrequested network callbacks.
- No hidden/background audio capture, upload, or cloud synchronization.
- Keep projects, audio, exports, logs, and recovery data local by default. External destinations require an explicit user-initiated action.
- Do not download dependencies or bootstrap packages during CMake configuration. Use already installed dependencies and document them.
- Audio callbacks must not allocate, lock, log, perform disk/network I/O, or block. Verify atomic lock-free suitability on target platforms before making hard realtime claims.
- UI/control work happens off the audio thread; communicate through bounded, validated, non-blocking command queues with documented single-producer/single-consumer ownership.
- Keep optional backends opt-in and avoid adding remote services.
- Never overwrite source archives or delete legacy assets without inventory and explicit rationale.
- Do not claim a test, build, hardware run, zero-xrun result, soak result, privacy proof, or release readiness unless the corresponding evidence exists. Mark unrun target-machine checks `not_run`.

## Engineering behavior

- Inspect before editing; use repository APIs instead of inventing parallel architectures.
- Prefer complete, small vertical slices over broad non-compiling rewrites.
- Make each change buildable and covered by regression tests where feasible.
- If a gate depends on physical hardware or unavailable dependencies, implement the harness/documentation and clearly leave the hardware result open; do not fabricate it.
- Do not merge or release merely to make a status badge green. Preserve reviewability and report blockers plainly.
- Keep UI labels and status text truthful: distinguish project edits, queued engine commands, active audio, and offline/reference rendering.
- On each phase, update the ledger so no feature is represented as complete before its acceptance evidence exists.

## Definition of done

A feature is complete only when source implementation, relevant tests, build registration, documentation, and honest status evidence agree. Overall release readiness additionally requires target-device startup/shutdown, callback timing, lock-free checks, latency/xrun testing, performance/soak evidence, and privacy/dependency review on a named target system.

**Instruction:** All implementation gates are open. Continue automatically through the phases above; ask only when a decision is genuinely ambiguous, destructive, requires a secret/credential, changes external account state, or needs physical access that is unavailable.
