# Voice processor audit

## Status

**Source inspection only; baseline build and timing benchmark have not been run in this workflow.** No SIMD speedup, broadcast-quality result, allocation-guard result, sanitizer result, or target-hardware result is claimed. This audit is intentionally a baseline, not a statement that the requested voice chain already exists.

## Current processing path

The closest existing realtime voice/mix path is `RealtimeTrackMixer` in `include/hush/realtime_track_mixer.hpp` and `src/realtime_track_mixer.cpp`. Its current state consists of per-track gain, pan, and mute/solo flags, plus a fixed-capacity SPSC command queue. `configure(track_count, max_frames)` allocates the per-track vectors on the control thread. `process()` drains queued control snapshots at the start of a block, computes whether any track is soloed, clears the stereo outputs, calculates equal-power pan gains, and sums each mono track into left/right outputs.

The existing processor is therefore gain/pan/mute/solo mixing—not a broadcast voice chain. There is no built-in high-pass filter, band-limited de-esser, broadband compressor, parametric EQ, or output limiter in this path.

## Block processing and realtime constraints

The public `process()` API receives a contiguous block (`frames`) and has no virtual dispatch in its sample inner loop. The implementation is block-based at the API boundary, but its work is nested as track → frame, with a per-track gain/pan calculation and mute/solo branch outside the frame loop. The per-frame loop performs input reads and stereo accumulations. No function pointer or virtual call is used in that inner loop.

The implementation comment and source indicate that `process()` performs no allocation, locking, logging, or I/O. This has not been verified with an allocation-hook test in this task. Do not move filter construction, vector resizing, file access, or parameter queue allocation into this function.

Other relevant paths: `PreallocatedBusProcessor::process()` processes configured bus buffers; `RealtimeBusEngine::process()` consumes the bounded fader queue and fills preallocated scratch; `RealtimeBusAudioAdapter::process()` converts interleaved stereo buffers to preallocated planar scratch. None of those classes is itself a broadcast voice chain.

## SIMD status

Source inspection of the current `RealtimeTrackMixer` shows no explicit AVX/AVX2 intrinsics and no OpenMP SIMD pragma. The frame loop is contiguous and may be eligible for compiler auto-vectorization, but that is compiler/build/target dependent. No vectorization report, assembly inspection, or profiling output has been produced here. Therefore **SIMD is not claimed**.

The requested benchmark harness is `tools/hush_voice_path_benchmark.cpp`. It times ten million frames by default through the existing one-track mixer in 512-frame blocks, with allocation/configuration outside the timed region. Run it in an optimized build and record the emitted JSON along with compiler, CPU, flags, and commit SHA. The harness is a baseline only; it does not compare scalar, auto-vectorized, and AVX2 variants.

## Sample-rate and precision target

Proposed target: **48 kHz internal processing, 32-bit float samples**. This matches the requested broadcast-oriented design target, but the current `RealtimeTrackMixer` API does not own or enforce a sample rate; it processes the number of frames passed by its caller. No resampler or 96 kHz input-conversion path was identified in the inspected mixer API. Therefore the 48 kHz target is a design requirement, not an implemented/validated engine guarantee. 96 kHz input acceptance must not be claimed until a resampling path and tests exist.

A 24-bit delivery file is a separate export/quantization decision. Float32 internal processing does not itself prove a 48 kHz / 24-bit broadcast deliverable.

## Gaps before a broadcast voice chain can be claimed

1. Four-pole (24 dB/octave) high-pass filter, default 80 Hz.
2. De-esser that attenuates a controlled 5–10 kHz sibilance band rather than applying broadband gain reduction.
3. Smooth broadband compressor with documented threshold, ratio, makeup gain, and attack/release behavior.
4. Parametric EQ with low shelf, two peaking bands, and high shelf.
5. Output limiter with bounded output and tested behavior.
6. Block-boundary parameter updates through a bounded queue, with all filter/envelope state allocated before audio starts.
7. Per-stage null/neutral tests, deterministic tests, frequency-response tests, sibilance discrimination, compressor curve tests, and an allocation guard.
8. Benchmarks comparing the baseline, compiler auto-vectorization, and any proposed AVX2 implementation; retain AVX2 only if measured faster on the documented target.

No look-ahead should be added unless an existing latency-compensation design can account for it. AVX2 gather is not proposed for recursive filter state.

## Validation ledger

| Item | Status |
|---|---|
| Source-level current-path audit | Completed by inspection |
| Baseline configure/build/CTest | Not run in this workflow; no local shell/clone available through the GitHub editing connector |
| 10-million-frame baseline benchmark | Harness added; not run |
| Compiler vectorization report | Not run |
| Scalar vs auto-vectorized vs AVX2 comparison | Not run |
| Null/determinism/frequency/compressor/de-esser tests | Not implemented |
| Audio-path allocation guard | Not implemented |
| ASan/TSan for voice processing | Not run |
| Physical audio hardware validation | Not run |

## Non-claims

This change does not implement or validate a broadcast-quality processor, 96 kHz internal processing, 96-to-48 kHz resampling, AVX2 acceleration, an allocation-free guarantee beyond the existing source contract, or commercial broadcast equivalence. The separate 500-track performance gate and 8-hour soak remain unrelated and open.
