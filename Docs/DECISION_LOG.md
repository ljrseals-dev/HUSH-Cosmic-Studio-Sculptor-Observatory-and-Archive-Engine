# HUSH decision log
## 0.2 foundation
1. Headless first: follows Docs/Prompt; no GUI until the 0.2 gates pass.
2. Language/build: C++20 and CMake 3.20+, with no dependency downloads or network build steps.
3. Project model: schema version 1, stable string IDs, explicit model API. Schema changes require tests and a migration plan.
4. Pan law: default equal-power −3 dB center; selectable legacy −4.5 dB center.
5. Audio sample contract: internal float32. The mixer helper does not clamp floating-point samples and does not dither.
6. Undo: append-only operation entries with inverse operations.
7. Real-time safety: audio callback must not allocate, lock, log, or perform file I/O.
8. Memory reclamation: graph swapping is deferred until a concrete safe reclamation strategy is implemented and tested.
9. Privacy: no telemetry, analytics, update checks, runtime network access, or remote configuration.

## 0.2 bus-routing foundation
10. Directed acyclic bus graph: routes point from source to destination; cycles and self-routes are rejected before render order is exposed.
11. Send tap metadata: pre-fader and post-fader taps are represented; audio summing semantics are covered by offline and preallocated reference tests.
12. Graph validation uses standard containers on the setup/control side only and must not run inside the audio callback.

## 0.2 preallocated processor
13. Bus topology and route IDs are resolved to numeric indices on the control thread; the processing path uses preallocated buffers and does not perform graph lookups.
14. The first processor API returns enum errors on the process path to avoid diagnostic-string allocation. Configuration and object lifetime remain host responsibilities.
15. Live graph replacement is still blocked on safe reclamation; do not atomically swap owning pointers and destroy the old graph on the audio thread.

## 0.2 parameter-to-processor integration
16. `RealtimeBusEngine` drains a bounded SPSC queue at block boundaries and applies the final queued fader value per bus before calling the preallocated processor. Queue overflow is explicit (`enqueue_fader_change` returns false); callers must handle rejected changes.
17. The integrated engine is configure-once. Live topology replacement remains blocked until safe graph reclamation is implemented; this component is not a device backend and does not claim target-hardware real-time readiness.


## 0.3a mixer GUI foundation
18. The first desktop UI is an optional Qt 6 Widgets target; Qt is supplied by the operating system and is never downloaded by CMake. This keeps the headless C++20 build unchanged for users who do not request the GUI.
19. The GUI uses the canonical schema-v1 project reader/writer and records track control edits through OperationLog::apply_and_append. GUI edits currently change saved project state only; live audio graph binding is explicitly not claimed.
