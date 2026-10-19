# HUSH 0.2 — Bus-routing foundation
## Implemented in this change
- Explicit bus entities and directed routes.
- Pre-fader/post-fader tap metadata on each route.
- Validation for empty/duplicate bus IDs, unknown endpoints, self-routes, invalid gain, and directed cycles.
- Source-before-destination render order for valid graphs.
- Contract tests for valid routing and rejected invalid graphs.

## Boundaries
This is a model/validation foundation, not a real-time audio graph. It does not yet sum bus buffers, implement pre/post-fader tap behavior, add track sends, detect cycles across persisted project entities, or publish/reclaim graphs lock-free. The current project JSON schema does not serialize bus entities yet.

## Remaining 0.2 gates
Autosave and durable crash journal/replay; ALSA streaming; preallocated processing graph and bounded parameter queue; safe graph swap/reclamation; real bus rendering and meters/xrun readout; hardware latency/xrun/soak and large-project performance validation.
