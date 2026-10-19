# Changelog

Notable repository changes. This is a development changelog, not a claim of a finished DAW release.

## 0.2.0 — headless foundation

- C++20 CMake project with schema-v1 persistence, bus graph validation, offline/reference rendering, preallocated bus processing, and a bounded SPSC parameter queue.
- Added realtime stereo adapter and optional ALSA backend/host.
- Added recovery journal, autosave policy controller, and operation-log undo/redo primitives.
- Added optional Qt 6 mixer shell; project-state editing is not live audio control.
- Added CI for default C++20, ALSA-enabled, Qt 6 GUI, ASan, and TSan configurations.
- Added release-gate ledger and documented unverified hardware/performance/durability gates.

## Unreleased — repository hygiene

- Added a tracked-file and archive audit in `Docs/REPO_AUDIT.md`.
- Removed malformed 2-byte ZIP placeholders and an unreferenced image; preserved non-empty archives whose contents could not be verified.
- Added artifact-reintroduction CI guard, contribution guidance, and security reporting policy.
- Published history remains intact; no filter-repo, force-push, or history rewrite was performed.
