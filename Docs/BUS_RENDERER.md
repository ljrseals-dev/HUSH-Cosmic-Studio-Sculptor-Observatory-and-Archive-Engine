# Bus-buffer reference renderer

The renderer converts a validated `BusGraph` plus one stereo direct-input buffer per bus into rendered pre-fader and post-fader buffers.

## Send semantics

- A bus' pre-fader buffer starts with its supplied direct input and accumulates sends from upstream buses.
- The bus fader multiplies the pre-fader buffer to produce its post-fader buffer.
- A `PreFader` route sends the source pre-fader buffer; a `PostFader` route sends the source post-fader buffer.
- Each send applies its route gain and adds into the destination's pre-fader buffer.
- A valid acyclic graph's topological render order ensures upstream sends are accumulated before a destination's fader is applied.

## Contract and limitations

The function rejects invalid/unconfigured graphs, missing/duplicate bus inputs, mismatched channel lengths or frame counts, and non-finite/out-of-range fader gains. Route validation (including route gains and cycles) remains the responsibility of `BusGraph::configure`.

This is an offline/reference implementation. It allocates vectors and uses lookup containers during rendering; do not call it from an audio callback. It is not yet wired to project tracks, inserts, a device backend, or a preallocated real-time graph. Those remain separate milestones.
