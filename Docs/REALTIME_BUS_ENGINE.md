# Realtime bus engine integration

`RealtimeBusEngine` composes the existing preallocated bus processor with the fixed-capacity SPSC parameter queue.

## Lifecycle and thread contract

1. Construct and call `configure(graph, max_frames, error)` on a control/setup thread.
2. Configure once only. The graph topology and all buffers are fixed for this object's lifetime.
3. One control/UI producer may call `enqueue_fader_change(bus_index, gain)`; the queue is bounded at 1024 entries. Invalid bus indices and gains outside [0, 16] are rejected. A full queue returns false; callers should retain/retry or surface the rejected update.
4. One audio-thread consumer calls `process(inputs, count, frames)`. Inputs follow `BusGraph::buses()` order. Queued fader changes are drained in FIFO order at the start of a valid processing call; if multiple changes target one bus in the same batch, the last one wins.
5. Query output views only when processing is not concurrently mutating them, unless the caller guarantees a safe read arrangement.

## Real-time properties and limits

- Scratch input views and fader state are allocated during configuration. The processing call performs no explicit allocation, lock, logging, or I/O.
- The SPSC queue requires exactly one producer and one consumer; it is not a multi-producer queue.
- Check `parameter_atomics_are_lock_free()` on the target. It reports the queue's atomic properties; it does not certify the whole application as real-time safe.
- This is not an audio-device backend. It does not open ALSA/JACK/PipeWire, run a callback, count xruns, provide safe graph replacement, or integrate track assignment/inserts/solo/mute/meters.
- Configure-once avoids unsafe live reconfiguration. To change topology, stop processing and destroy/recreate the engine under host-controlled synchronization. A lock-free graph-swap and reclamation protocol remains a separate required milestone.
- No target-hardware real-time, latency, underrun, or long-soak claim is made by this component's unit tests.
