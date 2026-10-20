# Track-capacity audit — effectively large, bounded sessions

## Status

**Baseline prerequisite passed in CI; the bounded-capacity core and structural track controls are implemented on this branch, with fresh CI validation pending.** See [BUILD_LOG.md](BUILD_LOG.md). The baseline revision passed default C++20, ALSA, Qt, Qt+ALSA, ASan, and TSan in CI run 39449869574. The first capacity implementation passed those same CI configurations in run 39451685619. These are CI results, not local-shell runs.

**No existing tracks, project data, source files, archives, or user assets are to be deleted or rewritten as part of this task.** Existing archive disposition remains a separate review gate.

## Strategy decision

Choose **Strategy A: a documented hard ceiling**, `HUSH_MAX_TRACKS=1024` by default, configurable up to 4096. The default is the supported product target; higher values increase preallocated live-monitor memory. This is not mathematically unlimited. It is a predictable maximum, with rejection and a visible non-blocking error at the limit. Do not implement a different strategy without an explicit owner decision.

Why A: the realtime track mixer already configures per-track state before processing and processes with preallocated vectors. A hard ceiling is the least invasive fit for the no-allocation callback rule. It avoids growing track state while audio is running. It does not establish that 1024 tracks meet any performance budget.

## Source findings

### Project model and persistence

- `include/hush/project.hpp` stores tracks in `std::vector<Track> tracks`; the `Track` model has ID/name strings, gain, pan, mute, and solo. The inspected declaration does not itself define a track-count ceiling.
- The project model declares schema version 1. This task must not add a persisted field or change serialization semantics casually. Schema-v1 reading and round-trip compatibility must remain intact.
- The Qt mixer now exposes Add Track and Remove Last Track controls plus an N / HUSH_MAX_TRACKS indicator. Structural add/remove operations use the operation log and enqueue a single bounded realtime command that changes active track count at a block boundary; the track mixer is configured to the ceiling before ALSA starts. Core operation-log and block-boundary tests exist. Actual Qt widget-click/headless interaction tests are still open. Only the last track can be removed in this slice; arbitrary selected-track deletion and full track-source routing are not claimed.

### Realtime track mixer

- `include/hush/realtime_track_mixer.hpp` defines `RealtimeTrackMixer::kQueueCapacity = 2048`. This is the number of queued control commands, **not** the supported track count.
- `RealtimeTrackMixer::configure(track_count, max_frames)` allocates `gains_`, `pans_`, and `flags_` before processing. `src/realtime_track_mixer.cpp` currently rejects zero tracks and counts exceeding `uint32_t` index range, but has no product-level track ceiling.
- Per-track mixer state is approximately 9 bytes of element storage (one `float` gain, one `float` pan, one `uint8_t` flags), excluding vector capacity/allocator overhead and alignment. At 1024 tracks this is about 9 KiB of element payload. This is not a full-session RAM estimate.
- `RealtimeTrackMixer::process()` consumes queued controls, scans solo flags, clears output buffers, and sums the supplied mono inputs. The function is declared `noexcept` and uses preconfigured vectors; source inspection shows no explicit allocation, locking, logging, or file/network I/O in its body. This is not yet an allocation-guard test result.
- The caller owns and prepares the actual per-track input buffers and pointer array. At 128 frames, one mono float buffer per track is 512 KiB total for 1024 tracks (1024 × 128 × 4 bytes), excluding any additional buffers, routing, project strings, GUI objects, and allocator overhead. Actual full-engine memory must be measured on a named build/machine; no such measurement has been run.

### Bus graph and processor

- `BusGraph` stores buses, routes, and render order in dynamic vectors; it does not declare a fixed track count. Bus count is a separate topology dimension and must not be silently conflated with track count.
- `PreallocatedBusProcessor::configure()` allocates per-bus pre/post stereo buffers sized by bus count × max frames. Its `process()` contract explicitly forbids allocation, locking, logging, and I/O. There is no evidenced fixed bus-count ceiling in the inspected public header.
- `RealtimeBusEngine` preallocates input scratch and fader-gain vectors during configuration and uses a 1024-entry parameter queue. That queue bounds pending bus-fader commands, not tracks or buses.
- The inspected stereo adapters allocate left/right/silence scratch and bus-input views in `configure()`. They require stable topology/capacity while processing. A callback may not resize these vectors.

### Existing scale tools

- `tools/hush_routing_benchmark.cpp` accepts a synthetic track count up to 100000, but this is a tool input limit, not an engine guarantee or a 100000-track qualification.
- `tools/hush_headless_soak.cpp` currently exercises 500 synthetic tracks. Its existence is not an 8-hour soak result or a full-ceiling test.

## Capacity inventory

| Area | Current constraint/evidence | Meaning |
|---|---|---|
| Persisted project tracks | Dynamic `std::vector<Track>`; no explicit product ceiling in inspected declaration | Project-level validation must add a clear cap without breaking schema v1 |
| Realtime track mixer | Index encoded as `uint32_t`; current code allows counts up to that representational range, subject to allocation | No practical documented ceiling yet |
| Track-control queue | 2048 commands | Queue capacity, not track count |
| Realtime bus-fader queue | 1024 commands | Queue capacity, not bus count |
| Bus graph | Dynamic vectors for buses/routes/order | No fixed ceiling established by inspected header |
| Bus processor scratch | Per bus × configured frame count; sized at configuration | Allocation happens off callback; memory depends on bus count and frame capacity |
| Stereo adapters | Preallocated planar scratch and bus input views | Configuration must precede live processing |
| Benchmark tool | Maximum 100000 synthetic tracks accepted by CLI | Tool validation bound only, not engine capacity |

No track-count constant currently unifies these layers. Insert-slot capacity is not applicable to the inspected current engine: a production insert rack is not present in these interfaces, and no insert-slot limit should be invented.

## Realtime boundary

The inspected realtime entry points include `RealtimeTrackMixer::process()`, `PreallocatedBusProcessor::process()`, `RealtimeBusEngine::process()`, `RealtimeBusAudioAdapter::process()`, and `RealtimeBusEngineHost::process_to_stereo()`, plus the ALSA callback adapter. Their contracts require preallocation and prohibit allocation/blocking/file or network I/O on the audio thread. New track creation, project serialization, vector growth, graph construction, GUI updates, and user-facing error construction must remain on a control thread.

The required allocation guard must test the actual processing path at full configured capacity. Comments and source inspection alone do not prove the property.

## Memory budget and measurement plan

- Known per-track mixer element payload: about 9 bytes × track count, excluding alignment and container overhead.
- Caller-owned mono float input payload at 128 frames: 512 KiB for 1024 tracks; this is a concrete illustrative buffer budget, not a claim about the repository's actual full engine configuration.
- Bus scratch memory depends on bus count and maximum frame count: four float arrays per bus in `PreallocatedBusProcessor::RuntimeBus` (pre-left, pre-right, post-left, post-right), or approximately 16 × bus_count × max_frames bytes for those samples, excluding object/vector overhead.
- Full project track strings, adapter buffers, bus runtime state, GUI state, allocator overhead, and peak memory during graph replacement are not yet measured.
- Do not mark the memory gate complete until measurements are made for startup and maximum configured capacity on a named machine and exact commit.

## Implementation progress and remaining work

1. [x] Add the shared `HUSH_MAX_TRACKS` compile definition with default 1024, CMake cache configurability, and a 4096 maximum to bound live-monitor memory.
2. [x] Reject over-ceiling schema-v1 project loads with `track_count_exceeded`; existing schema-v1 format is unchanged.
3. [x] Add boundary coverage at exactly the ceiling and ceiling+1. The test uses the configured default ceiling; separate runtime tests for 1, 8, and 64 are still required.
4. [x] Add an `operator new` allocation guard around `RealtimeTrackMixer::process()` at the full configured ceiling. This does not prove every audio path is allocation-free.
5. [x] Add undoable Add Track / Remove Last Track actions, a track-count indicator, non-blocking ceiling feedback, and block-boundary active-count commands. [ ] Add headless Qt widget-interaction tests; current tests exercise the core operation log and mixer command path.
6. [ ] CLI parity remains open: the current ALSA CLI host is a bus-graph host, not the GUI's realtime track mixer, so adding flags alone would not create genuine engine parity.
7. [ ] Extend graph-swap stress coverage at the chosen ceiling under ASan and TSan.
8. [ ] Run the 1024-track offline renderer correctness test and measure startup/full-capacity memory. Keep the 500-track performance budget, 8-hour soak, and physical hardware gates open until their own evidence exists.
9. [ ] Update README, implementation status, release gates, roadmap, and changelog to reflect verified progress.

## Stop condition

The baseline stop condition has been satisfied by CI. Do not claim the full track-capacity prompt complete: headless Qt interaction tests, CLI parity, full-ceiling graph-swap stress, measured memory, 500-track performance qualification, 8-hour soak, and hardware validation remain open.
