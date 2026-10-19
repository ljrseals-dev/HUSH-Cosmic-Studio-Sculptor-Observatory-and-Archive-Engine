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
11. Send tap metadata: pre-fader and post-fader taps are represented; audio summing semantics remain deferred until the processing graph exists.
12. Graph validation uses standard containers on the setup/control side only and must not run inside the audio callback.
