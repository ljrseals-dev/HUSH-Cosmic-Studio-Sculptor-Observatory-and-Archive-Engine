# Gate 3 routing audit

Audit basis: source files fetched from branch `feat/gui-live-mixer-wiring` before routing changes. This is a source audit, not a local configure/build/test result. Line references below point to the current branch and should be rechecked if the branch changes.

## 1. Current graph shape

- `include/hush/project.hpp:15-31` defines `Track` with `id`, `name`, `gain`, `pan`, `mute`, and `solo`; `Project` is schema v1 and stores `tracks`, `buses`, and `routes`.
- `include/hush/bus_graph.hpp:7-22` declares `SendTap { PreFader, PostFader }`, `Bus { id, name }`, and `BusRoute { source_bus_id, destination_bus_id, tap, gain }`. Routes are bus-to-bus only. There is no track node, explicit master-node type, track-to-bus assignment, insert chain, or track/bus fader metadata in the graph model.
- `include/hush/preallocated_bus_processor.hpp:13-22, 49-74` accepts one `BusInputView` per bus, stores per-bus pre/post stereo scratch, and pre-resolves bus routes and render order on `configure()`. The processing API is allocation-free by contract, but its input is already aggregated per bus; it does not construct track sums or inserts.
- `src/bus_graph.cpp` validates bus IDs, route endpoints, self-routes, finite gain in [0,16], and directed cycles using DFS. The validator is control/setup-side; it is not a track graph.
- `src/bus_renderer.cpp` is an offline reference renderer and allocates maps and output vectors. It handles pre/post bus sends and must not be used on the realtime callback.

## 2. Current live path and hardcoded mapping

- The GUI's optional `LiveMixer` in `src/mixer_gui.cpp` configures `RealtimeTrackMixer` for the project's track count, creates mono track buffers before starting ALSA, and copies capture-left into track index 0 and capture-right into track index 1. Other tracks are zero-filled. The mapping is hardcoded in `LiveMixer::callback`; it is a monitor/demo mapping, not a general track-source router.
- The separate bus path is `RealtimeBusAudioAdapter` (`include/hush/realtime_bus_audio_adapter.hpp`, `src/realtime_bus_audio_adapter.cpp`). Its configure call takes fixed `input_bus_index` and `master_bus_index`; each device callback places the incoming stereo signal into the selected input bus and supplies silence to every other bus. `src/alsa_realtime_host.cpp` builds a two-bus graph `capture -> master` and configures adapter indices 0 and 1. This is direct capture-to-bus routing, not a track-to-bus path.

## 3. Current mute/solo behavior and precedence

- `include/hush/realtime_track_mixer.hpp:10-34` defines `TrackControlCommand` with gain/pan and bit flags (bit 0 mute, bit 1 solo); the mixer owns track-level state and applies queued snapshots inside its process block.
- The GUI sends track controls through `RealtimeTrackMixer`'s SPSC queue. These flags are not propagated into `BusGraph`, `PreallocatedBusProcessor`, or `RealtimeBusEngine`; therefore the bus-engine master path does not have end-to-end track solo/mute semantics.
- Source review of `src/realtime_track_mixer.cpp` is required before declaring exact precedence. This audit intentionally does not infer precedence from the field names alone. Gate 3 must encode the selected rule in a named helper and test the complete permutation matrix. Recommended explicit rule for this implementation: **mute wins**—a muted track contributes silence even if soloed; when at least one unmuted track is soloed, only unmuted soloed tracks contribute. This avoids a solo toggle unexpectedly unmuting a track the user explicitly muted.

## 4. Missing full-mixer pieces and files to change

1. Track nodes and track-to-bus assignments: `include/hush/project.hpp`, `include/hush/bus_graph.hpp`, `src/project.cpp`, and the graph-building/configuration layer. Because project persistence is schema v1 and its reader explicitly accepts only version 1, persist new assignments/inserts only with a schema-v2 writer/reader and schema-v1 compatibility tests; do not silently add v1 fields.
2. Realtime track accumulation and multi-bus routing: `include/hush/preallocated_bus_processor.hpp`, `src/preallocated_bus_processor.cpp`, `include/hush/realtime_bus_engine.hpp`, `src/realtime_bus_engine.cpp`, and `include/hush/realtime_bus_audio_adapter.hpp` / `src/realtime_bus_audio_adapter.cpp`. Keep graph construction and buffer allocation off the audio thread.
3. End-to-end mute/solo: track-state command payloads and block-boundary state in the realtime engine; current `RealtimeBusEngine` queues only `BusFaderChange`.
4. Inserts: no insert interface or fixed-capacity insert storage exists in the inspected graph/processor headers. Only built-in, statically defined DSP blocks should be considered here; plugin hosting remains out of scope.
5. Track-to-bus and bus-to-master pre/post sends: current `BusRoute` supports pre/post taps only between buses. Extend route typing with track sources only under schema v2, and validate all route endpoints/cycles during graph build.
6. Per-node meters: no peak/RMS meter data or publication API appears in the inspected processor/engine headers. Add fixed-size meter state to each preallocated node and publish coherent snapshots for non-audio readers; GUI polling must not read mutable audio-owned non-atomic fields.
7. Xrun counters: `include/hush/alsa_audio_backend.hpp` already exposes `xrun_count()` backed by atomic `xruns_`; `src/alsa_audio_backend.cpp` increments it for negative ALSA reads/writes, short capture transfers, and zero-length writes. The host prints callback/recovery summary after stop. Missing evidence is a simulated backend error-path test; current implementation is coupled directly to ALSA calls and no injectable write/read function is exposed. The counter counts observed recovery/short-transfer outcomes, not only literal ALSA `-EPIPE` underruns.

## 5. Meter and xrun attachment points

- Meters should be computed in the preallocated processor after the final sample values for each node/tap are known, with fixed storage allocated in `configure()`. Publish a POD snapshot through an explicitly synchronized lock-free mechanism; the GUI reads only published snapshots on its GUI thread.
- Xrun counts belong to `AlsaAudioBackend` because only that layer sees ALSA return codes. Its atomic accessor is already GUI-readable. Add a simulated failure test through an injectable ALSA I/O seam or narrow function table, without changing callback-thread constraints. Host summary belongs after `backend.stop()` in `src/alsa_realtime_host.cpp`.

## 6. Schema impact

The current `Project` contains `schema_version{1}`, tracks, buses, and bus-only routes (`include/hush/project.hpp:20-31`). Existing `src/project.cpp` writes buses/routes and reads schema version exactly `1`, with bus and route arrays optional for older v1 files. Track-to-bus assignment and persisted insert chains require a schema-v2 model. Suggested v2 fields: each track has `output_bus_id` (or a routing list if multi-destination sends are supported); route records identify a typed source (`track_id` or `bus_id`) and destination bus plus tap/gain; inserts are ordered built-in insert descriptors with stable type IDs and parameters. The v1 reader must retain the old default routing behavior and v1 round-trip fixtures must remain unchanged. Do not implement v2 without backward-compatible read tests and v1/v2 round-trip tests.

## 7. Existing tests identified

- `tests/bus_graph_tests.cpp`: bus graph validation and cycle/endpoint checks (verify exact test names in source before editing).
- `tests/bus_renderer_tests.cpp`: offline reference rendering and pre/post bus sends.
- `tests/preallocated_bus_processor_tests.cpp`: preallocated processor contract.
- `tests/realtime_bus_engine_tests.cpp`: block-boundary fader command, invalid input, bounded queue.
- `tests/realtime_bus_audio_adapter_tests.cpp`: interleaved stereo adapter.
- `tests/project_tests.cpp`: schema-v1 persistence and compatibility.
- `tests/realtime_track_mixer_tests.cpp`: track mixer command behavior, including mute/solo; exact precedence must be confirmed from implementation and tests.
- `tests/realtime_graph_publisher_tests.cpp`: held-reader reclamation and concurrent publication stress.
- `tests/alsa_audio_backend_tests.cpp` (if present in the branch): check exact file inventory; the backend currently has no obvious injectable `snd_pcm_writei` stub in the inspected source.
- CI workflow: `.github/workflows/hush-0.2.yml`. The last cited green run is an earlier revision; any Gate 3 commits require fresh CI. No local build or target-device validation is claimed by this audit.

## Gate 3 decision

The existing engine is a bus-only graph plus a separate track mixer; the two are not integrated. A complete implementation requires a typed routing model and schema-v2 persistence for track assignments/inserts, so the first safe step is a tested schema/graph design and baseline evidence. The current remote GitHub interface does not provide a local terminal or the user's clone, so the three exact local baseline command outputs cannot be honestly recorded as executed here. They must be run in the clone before treating the gate as closed. No hardware, zero-xrun, latency, soak, or power-loss claim is made.
