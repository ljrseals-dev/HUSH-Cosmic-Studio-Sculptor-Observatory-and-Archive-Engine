# Realtime track-to-bus router

## Scope

`RealtimeTrackBusRouter` is the preallocated first stage of the live track → bus → master pipeline. Configure it on the control thread with one bus index per mono track, the bus count, and the maximum block size. The returned `BusAudioInputView` array can be passed directly to `RealtimeBusEngine::process()`; the existing bus graph then handles bus-to-bus routes and the master bus.

Track assignment is an explicit runtime configuration for now. Schema v1 has no persisted per-track output-bus field, so this stage does **not** invent one, alter project files, or claim persisted GUI routing.

## Realtime contract

- Configure once before audio starts; all scratch buffers are allocated during configuration.
- One control-thread producer calls `enqueue_control()`; the audio-thread consumer calls `process()`.
- Gain, pan, mute, and solo snapshots are bounded SPSC commands applied at the start of a processing block.
- Solo is global across tracks: when any track is soloed, only soloed and unmuted tracks contribute to their assigned buses.
- Queue saturation returns `false`; no waiting, allocation, logging, or I/O is performed in `process()`.
- Bus-to-bus sends and pre-/post-fader taps remain the responsibility of `BusGraph` / `PreallocatedBusProcessor`.

## Automated coverage

`hush_realtime_track_bus_router_tests` checks invalid mappings, deterministic multi-track summing to two buses, mute, global solo, and invalid block sizes. CI results on the commit that adds this target are authoritative; this source-level test is not hardware loopback evidence.

## Remaining integration

The router is a tested core primitive, not yet the Qt/ALSA live path. Full phase 3 still requires canonical project routing semantics, inserts where supported, end-to-end tests through the published bus engine and device-output adapter, GUI meters/xrun reporting, and hardware validation. No claim of complete playback-capable DAW is made.
