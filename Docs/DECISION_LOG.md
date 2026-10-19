# HUSH decision log

## 0.2 foundation
1. **Headless first:** follows `Docs/Prompt`; no GUI until the 0.2 gates pass.
2. **Language/build:** C++20 and CMake 3.20+, with no dependency downloads or network build steps.
3. **Project model:** schema version 1, stable string IDs, explicit model API. Schema changes require tests and a migration plan.
4. **Pan law:** default equal-power −3 dB center; selectable legacy −4.5 dB center.
5. **Audio sample contract:** internal float32. The mixer helper does not clamp floating-point samples and does not dither.
6. **Undo:** append-only operation entries with inverse operations; mutations are represented as model API operations, not GUI-specific state.
7. **Real-time safety:** the eventual audio callback must not allocate, lock, log, or perform file I/O. The mixer primitive is not yet a complete device callback.
8. **Memory reclamation:** graph swapping is deferred until a concrete epoch/deferred-free strategy is implemented and tested; no unsafe raw-pointer swap is claimed.
9. **Privacy:** no telemetry, analytics, update checks, runtime network access, or remote configuration is included.
