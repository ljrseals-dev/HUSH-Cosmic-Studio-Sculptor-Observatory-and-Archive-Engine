# Changelog

Notable repository changes. This is a development changelog, not a claim of a finished DAW release.

## 0.2.0 — headless foundation

- C++20 CMake project with schema-v1 persistence, bus graph validation, offline reference rendering, preallocated bus processing, and a bounded SPSC parameter queue.
- Added realtime stereo adapter and optional ALSA backend/host.
- Added recovery journal, autosave policy controller, and operation-log undo/redo primitives.
- Added optional Qt 6 mixer shell; project-state editing is not live audio control.
- Added CI for default C++20, ALSA-enabled, Qt 6 GUI, ASan, and TSan configurations.
- Added release-gate ledger and documented unverified hardware/performance/durability gates.

## Unreleased — transport and automation core (initial implementation; validation pending)

- Added a fixed-capacity SPSC transport command queue for play, stop, pause, and sample-position seek, with block-boundary command application and a sample clock.
- Added a bounded tempo-map utility with sample/beat conversion and ordered tempo points.
- Added an automation-lane utility with step/linear interpolation and endpoint hold.
- Registered initial transport/automation contract tests in CMake.
- Not yet connected to the running audio callback, Qt transport controls, project persistence/schema migration, or OperationLog. Build and test results are pending; no production-readiness claim is made.

## Unreleased — Phase 1–7 execution tooling

- Added configurable synthetic routing benchmark and headless mixer/recovery-journal soak executables with JSON output.
- Added recovery journal tests for interrupted record headers and truncated payloads. These additions have not yet been run in this session; performance and soak reports remain outstanding.

## Unreleased — GUI engine wiring audit

- Added a widget-by-widget audit of the Qt mixer shell and the existing track/bus SPSC command payloads.
- Recorded the blocker: the Qt executable has no running audio host/callback consuming track-mixer commands, and project tracks do not yet provide audio buffers. The live-wiring gate remains open; no fake timer-based or queue-only integration is claimed.
- Linked the audit from the mixer GUI documentation and README. No live GUI wiring tests have been added yet.

## Unreleased — repository hygiene

- Added tracked-file, historical rename, and archive audit in `Docs/REPO_AUDIT.md`.
- Removed four malformed 2-byte ZIP placeholders and the unreferenced `images/1792375771560.jpg`.
- Preserved two non-empty archives whose full contents could not be verified; they remain explicit audit debt and must not be treated as source dependencies.
- Added artifact ignore rules, contribution guidance, and security reporting policy.
- Published history remains intact; no filter-repo, force-push, or history rewrite was performed.
