# Bounded SPSC parameter queue

`hush::SpscParameterQueue<T, Capacity>` is a fixed-capacity, allocation-free queue intended to transfer small trivially-copyable parameter commands from one control thread to one consumer (eventually the audio thread).

## Contract

- Exactly one producer may call `try_push`; exactly one consumer may call `try_pop`. This is not an MPSC/MPMC queue.
- Full and empty conditions return `false`; no waiting, retry loop, allocation, or logging occurs in either operation.
- Release/acquire publication protects each slot between producer and consumer.
- Capacity is fixed at compile time. The queue does not overwrite old commands when full.
- Payloads must be trivially copyable. Use stable parameter IDs and scalar values; do not enqueue owning pointers or dynamically-managed objects.
- `size_approx()` is diagnostic only and must not be used to coordinate threads.
- `atomics_are_lock_free()` reports whether the index atomics are lock-free on the current target. Validate this on supported hardware before making real-time guarantees.

## Limits

This queue is a transport primitive, not a complete parameter system. It is not yet connected to an audio callback, parameter validation/smoothing, a processing graph, or graph replacement/reclamation. The tests cover deterministic contract and wrap-around behavior in a single thread; they do not constitute a concurrent stress test or prove zero xruns on a target machine.
