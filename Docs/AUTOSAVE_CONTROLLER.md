# HUSH 0.2 — Autosave controller contract

## Host integration

`hush::AutosaveController` is a scheduler policy, not a background thread. The host must call `poll(project, steady_clock::now(), transport_running, error)` at a regular cadence from a non-audio thread (for example, its main/control loop). It writes a snapshot on the first poll, when the configured interval elapses, and on a transport-running to transport-stopped transition. The default interval is two minutes; non-positive intervals are clamped to one millisecond.

Outcomes are `Saved`, `NotDue`, and `Failed`. A failed write is surfaced to the caller; the controller does not silently report success. The host should present/log the error and continue polling so a later attempt can succeed. The host must call after project mutations are committed to the canonical model.

## Threading and limits

Do not call this API from an audio callback: serialization and filesystem I/O can allocate and block. The class owns no worker thread, timer, UI, or transport; it must be integrated by the application. Transport-stop saves happen on the next poll observing the transition. The journal requests file sync, but power-loss durability still depends on filesystem and hardware behavior. This does not replace kill/restart testing on target systems.
