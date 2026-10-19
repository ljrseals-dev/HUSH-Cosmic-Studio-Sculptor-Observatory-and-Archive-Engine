# HUSH 0.2 — Bus-routing foundation
## Implemented
- Explicit bus entities and directed routes.
- Pre-fader/post-fader tap metadata on each route.
- Validation for empty/duplicate bus IDs, unknown endpoints, self-routes, invalid gain, and directed cycles.
- Source-before-destination render order for valid graphs.
- Contract tests for valid routing and rejected invalid graphs.
- Canonical schema-v1 project JSON now serializes `buses` and `routes`; older schema-v1 documents without these fields still load.
- Project loading validates persisted routing before accepting the project.

## Boundaries
This remains a model/validation foundation, not a real-time audio graph. It does not yet sum bus buffers, apply pre/post-fader tap behavior, assign tracks to buses, or publish/reclaim graphs lock-free. The project parser is the repository's existing constrained schema reader, not a general-purpose JSON implementation.

## Remaining 0.2 gates
Autosave scheduler and lifecycle integration; crash/restart integration testing; ALSA streaming; preallocated processing graph and bounded parameter queue; safe graph swap/reclamation; real bus rendering and meters/xrun readout; hardware latency/xrun/soak and large-project performance validation.
