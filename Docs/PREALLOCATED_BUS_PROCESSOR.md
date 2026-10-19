# Preallocated bus processor

The `hush::PreallocatedBusProcessor` is the first real-time-oriented processing path for the HUSH 0.2 headless foundation. `configure()` is a control-thread operation and may allocate. `process()` uses buffers and route indices prepared during configuration and performs no explicit allocation, locking, logging, or file I/O.

## Usage contract

- Configure a valid `BusGraph` and a maximum frame count away from the audio callback.
- Provide exactly one `BusInputView` per graph bus, in the same order as `BusGraph::buses()`.
- Input pointers must remain valid for the full call and address at least `frame_count` samples.
- One thread may call `process()` for a given processor instance at a time.
- Keep configuration, destruction, and reconfiguration off the audio thread.
- `process()` returns a small enum status rather than allocating a diagnostic string on failure.
- Output views reference processor-owned buffers and remain valid only until the next call that processes the same bus. They are not thread-safe snapshots.

## Limitations

This is not yet a device backend, does not own track inputs, does not implement inserts, solo/mute integration, meters, or xrun counters, and does not solve live graph replacement/reclamation. The existing reference renderer remains useful for offline comparisons. Do not destroy or reconfigure a processor while another thread is processing it. No target-hardware zero-xrun claim is made.
