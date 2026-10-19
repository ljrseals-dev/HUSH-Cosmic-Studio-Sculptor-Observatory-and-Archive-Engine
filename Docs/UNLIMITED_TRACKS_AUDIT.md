# Effectively-unlimited track capacity audit

**Audit target:** `main` at `bb5fff7c06cb5ff5970733159590cba402636ceb` (2026-10-19).
**Status:** strategy recommended; implementation intentionally stopped at the baseline gate.
**Terminology:** this project does not currently have a declared "unlimited tracks" feature. A large configurable ceiling is a more accurate and testable contract.

## 1. Current model and limits

### Project track model

`include/hush/project.hpp` stores tracks as:

```cpp
std::vector<Track> tracks;
```

There is no `HUSH_MAX_TRACKS` constant or project-level maximum in this header. Schema version defaults to 1. The model can represent a variable number of tracks, but that does not establish that every runtime path can safely process an arbitrary number.

### Realtime track mixer

`include/hush/realtime_track_mixer.hpp` currently declares:

```cpp
static constexpr std::size_t kQueueCapacity = 2048;
std::vector<float> gains_;
std::vector<float> pans_;
std::vector<std::uint8_t> flags_;
SpscParameterQueue<TrackControlCommand, kQueueCapacity> commands_;
```

The 2048 value is a **parameter-command queue capacity**, not a track-count limit. `RealtimeTrackMixer::configure(track_count, max_frames)` allocates the per-track control arrays on the caller/control thread and currently accepts any positive count representable by its 32-bit command index, subject to memory allocation succeeding. The `process()` method iterates over the configured count and does not resize the arrays.

### Bus processor and bus engine

`include/hush/preallocated_bus_processor.hpp` has dynamic vectors for runtime buses, routes and render order. Its `RuntimeBus` has four frame buffers: pre-fader left/right and post-fader left/right. `configure()` sizes these from the validated graph and requested frame capacity. There is no compile-time bus-count ceiling in the inspected header. `include/hush/realtime_bus_engine.hpp` likewise uses dynamic input/fader scratch vectors sized during `configure()`; its parameter queue capacity is 1024. That 1024 is a **bus fader-command queue capacity**, not a bus or track count.

At 4096 frames, the bus processor's four float buffers cost 4 × 4096 × 4 = **65,536 bytes per bus** for sample storage alone, excluding IDs, vectors, routes, allocator overhead and other scratch.

### Device and GUI buffers

The current ALSA host uses `constexpr std::size_t max_frames = 4096` and configures the engine before opening the audio backend. The GUI's live mixer uses `LiveMixer::kMaxFrames = 4096` and allocates one 4096-float mono buffer per project track, plus a pointer per track and mixer control arrays, before starting ALSA. One mono buffer is 4096 × 4 = **16,384 bytes per track**. At 1024 tracks that GUI track-buffer pool alone is **16 MiB**, excluding control arrays, pointers, output buffers, allocator overhead, project/UI state, and bus graph storage.

The host adapters allocate their planar scratch and bus-input views in `configure()`, before the device starts. The inspected adapters have no track-count constant of their own; they rely on the engine's configured topology.

### Current limits summary

| Area | Current observed limit/behavior |
|---|---|
| Project track count | No explicit maximum in `Project`; `std::vector<Track>` |
| Realtime track mixer | Positive count; constrained by 32-bit track command index and available memory |
| Track-control SPSC queue | 2048 pending commands |
| Bus fader SPSC queue | 1024 pending commands |
| Bus count | No explicit compile-time cap in inspected model/processor headers; topology is dynamically configured |
| Bus processor sample storage | Four float buffers per bus, each `max_frames` long |
| ALSA host / GUI live mixer frame capacity | 4096 frames in current setup |
| GUI live track source mapping | Stereo capture feeds tracks 1–2 only; this is a routing limitation, not a track-count limit |
| Inserts / plugin slots | No insert-slot capacity was identified in the inspected files; plugin/insert routing is not a delivered live-path feature |

This is a source audit, not a claim that every path in every configuration has been fully measured.

## 2. Audio-thread allocation boundary

The source explicitly documents the relevant contracts:

- `include/hush/preallocated_bus_processor.hpp`: `configure()` may allocate; `process()` does not allocate, lock, log, or perform I/O.
- `include/hush/realtime_bus_engine.hpp`: configure on the control thread before publication; no allocation, locks, logging, or I/O in `process()`.
- `include/hush/realtime_bus_audio_adapter.hpp`: configure before device start; `process()` performs no allocation, locking, logging or I/O.
- `include/hush/realtime_bus_engine_host.hpp`: graph replacements are validated/configured off callback; `process_to_stereo()` is audio-thread-only and performs no allocation, lock, logging or I/O.
- `include/hush/realtime_track_mixer.hpp`: configure once on the control thread; `process()` performs no allocation, locking, logging or I/O.
- `src/mixer_gui.cpp`: the live mixer allocates its track buffers and control state before `backend_.start()`; the callback loops over prepared buffers and invokes the configured mixer.

These source comments and implementations establish the intended design boundary; they are **not yet a passing allocation-guard test at 1024 tracks**. Any capacity change must size or replace all affected storage on a non-realtime thread before publication/device start. The callback must never grow a vector or construct a replacement graph.

## 3. Track creation path

The project data model exposes a vector, but the current GUI source inspected here does not offer a complete Add Track/Remove Track workflow that updates the project, operation log, live mixer buffers, and audio engine consistently. The live mixer is configured from the project's current track count at Start. It cannot safely resize its callback-owned track buffers while running. This means track creation needs a control-thread operation and a defined stop/reconfigure/publish lifecycle, not a direct vector append while audio is active.

The current live capture source maps input left/right to the first two tracks. Raising the count alone will not implement per-track recording inputs or complete track-to-bus/master routing.

## 4. Strategy

**Recommended strategy: A — documented hard ceiling, default 1024 tracks.**

Rationale:
- Fits the existing configure-before-use and preallocation model.
- Makes allocation failure and capacity rejection observable on the control thread.
- Avoids adding dynamic growth or allocation to the audio callback.
- Gives the project a clear, testable claim instead of implying mathematical infinity.

Trade-off: 1024 is a ceiling, not truly unlimited capacity. Sessions above the ceiling must be rejected with a clear error. The owner can choose a different ceiling before implementation; this audit does not silently choose pooled growth or chunked processing.

## 5. Memory budget (estimates, not measurements)

At the current GUI frame capacity of 4096:
- One mono float track buffer: 16 KiB.
- 1024 GUI mono track buffers: 16 MiB.
- Track mixer state per track: gain + pan + flags, nominally about 9 bytes/track before allocator/alignment effects (about 9 KiB at 1024 tracks).
- Pointer table at 1024 tracks on a 64-bit target: about 8 KiB.
- Bus processor storage: 64 KiB per bus for four 4096-float buffers, before other storage.

The GUI's track-buffer estimate is a lower bound, not a whole-engine RAM budget. A full 1024-track project also requires project strings, operation/history state, UI widgets, audio inputs, routes, and possibly per-bus buffers. The repository's current live path does not create a 1024-track project end-to-end, so full-capacity memory must be measured after the feature exists. No measured startup or full-capacity RSS is claimed in this audit.

## 6. Baseline gate and stop condition

The required baseline commands are recorded verbatim in `Docs/BUILD_LOG.md`. They could not be run in the current execution environment because a local clone attempt failed at DNS/network resolution (`Could not resolve host: github.com`). GitHub source inspection is available, but it is not a substitute for the requested local default, ALSA, and GUI+ALSA configure/build/CTest runs.

Per the supplied prompt, implementation stops here until those baseline commands can be run in a network-capable local clone with the required dependencies. No source implementation changes, capacity claim, sanitizer result, or build/test pass is being asserted. The audit branch contains documentation only.

## 7. Required next steps after baseline is green

1. Run all three exact baseline command groups and append their verbatim output/status to `Docs/BUILD_LOG.md`.
2. Fix only baseline blockers and repeat until all three configurations pass.
3. Centralize `HUSH_MAX_TRACKS=1024` in the runtime/project/GUI configuration; reject count 1025 at project load and track creation.
4. Add tests at 1, 8, 64, 1024 and 1025 tracks, including a test-only allocation guard around the mixer processing path at 1024 tracks.
5. Wire undoable GUI add/remove operations through the established control/queue/safe reconfiguration path; add a count indicator and CLI parity.
6. Extend graph-swap stress tests under ASan/TSan and measure real startup/full-capacity memory.
7. Update README, implementation status, release gates, roadmap and changelog only after evidence exists. Keep the 500-track performance gate, eight-hour soak, and hardware validation open until their own reports exist.
