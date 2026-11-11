# Phase 5 — performance and soak gate

## Purpose

The benchmark and soak programs are deterministic offline software harnesses. They do not open an audio device and do not constitute hardware, latency-loopback, or zero-xrun validation.

## 500-track performance benchmark

Run the benchmark from a configured build:

```sh
./build/hush_routing_benchmark 500 10 2666.667 > routing-500.json
```

Arguments are track count, duration in seconds, and the p99 per-block processing budget in microseconds. Defaults are 500 tracks, 10 seconds, and 2666.667 µs (128 frames at 48 kHz). The JSON reports p95/p99/max measured mixer processing time, throughput, rejected parameter updates, and `hardware_audio_test: false`. Exit code 0 means the configured p99 budget passed; exit code 3 means it did not. Timing values are machine-dependent and must be retained with the tested commit and machine details before the gate is accepted.

The harness measures `RealtimeTrackMixer::process` duration. The synthetic benchmark does not include device scheduling, OS callback jitter, complete track-to-bus routing, or physical I/O. It is therefore a performance gate for this mixer path only.

## Eight-hour synthetic soak

Run:

```sh
./build/hush_headless_soak 28800 ./hush-soak.journal 64 > soak-8h.json
```

Arguments are duration in seconds, journal path, and maximum permitted positive RSS drift in MiB (default 64 MiB). It processes 500 synthetic tracks, exercises the bounded parameter queue, and writes periodic recovery snapshots. On Linux, RSS is read from `/proc/self/statm`; if RSS is unavailable, the report marks `rss_available: false` and the memory threshold cannot be treated as verified. A passing software result is not proof of leak-free behavior or hardware stability.

The output contains start/end RSS, positive drift, snapshot count, rejected updates, the configured threshold, and `hardware_audio_test: false`. Exit code 3 means the configured RSS drift threshold was exceeded.

## CI coverage versus qualification

CTest smoke-runs both harnesses at short duration to catch launch, JSON/status, and journal-write regressions. Smoke tests do not replace the full 500-track run or the 28,800-second soak. A qualifying run must preserve JSON output and record the commit SHA, OS/kernel, CPU, command, duration, configured threshold, and result. Do not claim either long gate passed until the corresponding full-duration report exists.


## CI 500-track report artifact

The Linux C++20 job now runs a 10-second, 500-track synthetic benchmark with the documented 2,666.667 µs p99 block budget and uploads `hush-500-track-performance-<run-id>` as a workflow artifact. The JSON includes the tested commit SHA, UTC timestamp, runner OS/kernel/CPU, exact command, benchmark result, and an explicit software-only qualification note. The job fails if the benchmark exits nonzero. This improves repeatable CI evidence and catches gross regressions, but shared-runner timing is noisy and is **not** a substitute for a qualifying report from the target machine. The eight-hour soak remains a separate required run.
