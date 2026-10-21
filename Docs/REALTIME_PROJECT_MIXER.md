# Realtime project mixer integration

`RealtimeProjectMixer` is the first composed core path joining persisted project bus topology, per-track runtime routing, and the published realtime bus engine. It is intentionally a core API; the Qt mixer and ALSA host are not yet switched to this composed path.

## Control-thread setup

The caller supplies one bus ID per track and the stable ID of the master bus. This is explicit by design: schema-v1 `Track` objects do not yet persist an individual bus assignment, so the wrapper does not invent one.

```cpp
hush::RealtimeProjectMixer mixer;
hush::Error error;
const std::vector<std::string> track_bus_ids{
    "drum-bus", "vocal-bus"
};
if (!mixer.configure(project, track_bus_ids, "master", 512, error)) {
    // Surface error.code / error.message on the control thread.
}
```

Configuration validates the project's `BusGraph`, verifies stable track IDs and every bus assignment, configures the published bus engine, allocates track-to-bus scratch, and queues initial gain/pan/mute/solo snapshots. Current initial seeding is bounded by `RealtimeTrackBusRouter::kQueueCapacity` (2048 tracks). A failed topology or invalid assignment is rejected; do not start audio unless configuration succeeds.

## Audio-thread processing

Pass one predecoded mono buffer per project track for the current block. The wrapper aggregates tracks into their assigned stereo bus inputs, then processes the bus graph and copies master output while the graph lifetime guard is held:

```cpp
const auto status = mixer.process(track_buffers, track_count, frames,
                                  output_left, output_right);
```

The processing method is preallocated and does not allocate, lock, log, or perform I/O. `enqueue_track_control()` is a single-producer control-thread API; commands are applied at the next block boundary. `reclaim_retired()` must only be called from the serialized control thread. Stop/join the audio reader before destroying the mixer or changing topology.

## Evidence and limits

- `hush_realtime_project_mixer_tests` covers invalid bus assignments, explicit mapping to separate buses, master summing, stable-ID control changes, block-boundary delivery, mute/solo precedence, and invalid frame/input counts.
- The test is synthetic-buffer contract coverage; fresh CI must pass before claiming the change test-backed.
- This does not yet add a persisted per-track bus ID, wire the composed class into the Qt GUI or ALSA device host, implement inserts, or provide graph-wide peak/RMS meters.
- Physical-device timing, latency, xrun and long-duration tests remain separate release gates.
