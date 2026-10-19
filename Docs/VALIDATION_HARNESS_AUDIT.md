# Validation harness audit

Audit target: `milestone/0.3b-validation-harnesses` base revision `4b59e14297e53b4bc749576f257d6b5734134ed6`.
This is a source review only. No local build or physical ALSA run was performed for this audit.

## ALSA backend

- `AlsaAudioBackend::start` calls `stop()`, validates callback/rate/period, opens capture then playback using `snd_pcm_open`, configures both for interleaved stereo float, checks matching rate/period, prepares both PCMs, allocates the period buffers before starting the worker, and launches a worker thread.
- Failure paths return `false` with an error string and close opened devices. Thread-creation failure is caught and surfaced.
- `stop()` requests shutdown, drops capture/playback to wake blocking I/O, joins the worker, clears callback state and buffers, and closes devices.
- `run()` calls `snd_pcm_readi` and `snd_pcm_writei`. Negative transfers call `snd_pcm_recover` and increment the xrun counter; short capture reads, zero writes, and failed/partial transfer conditions are also counted. A negative transfer that cannot recover terminates the worker. The host does not expose a structured terminal worker error.
- Device names are configurable through `AlsaAudioBackend::Config::{capture_device,playback_device}`, but the current host does not parse command-line options and uses `default`.
- The backend is **not currently stub-testable**: it directly calls ALSA functions, including open/read/write/recover/drop/close and PCM setup APIs. A seam must cover the operations used by the lifecycle, not only read/write, before deterministic `-EPIPE` and `-ENODEV` tests can exercise production control flow.
- Realtime callback itself is called after period buffers are allocated. The worker performs ALSA blocking I/O and error recovery outside the supplied process callback. No new logging or report I/O belongs in the callback.

## Host output and shutdown summary

`src/alsa_realtime_host.cpp` writes startup/configuration failures and the initial running message to stderr/stdout. After stop it prints callback count and the backend's recovery/short-transfer count to stdout. It does not write a report file, record per-period timing, record the kernel/CPU/device identity, or emit a machine-labelled shutdown JSON summary. It currently has no named-device command-line arguments.

## Existing tools and privacy audit

- `tools/privacy_source_audit.cmake`: scans `src/*.cpp`, `src/*.cc`, `include/*.hpp`, and `include/*.h` for a narrow denylist of network/telemetry/crash-reporting API markers. It does **not** scan `tools/`.
- `tools/hush_routing_benchmark.cpp`: synthetic `RealtimeTrackMixer` throughput test; positional track count and seconds; emits basic JSON to stdout; does not accept `--report`, enforce a configurable performance budget, or provide required machine metadata.
- `tools/hush_headless_soak.cpp`: synthetic mixer and recovery-journal loop; positional seconds and journal path; emits counters/RSS to stdout. It does not enforce a memory-drift bound, accept `--report`, or provide required machine metadata. Its default journal path writes a file in the working directory.
- CMake registers `hush_privacy_source_audit` as a CTest when `HUSH_BUILD_TESTS=ON`. Stress tools are built under `HUSH_BUILD_STRESS_TOOLS`, default ON, but are not currently test-injected ALSA harnesses.

## Existing report format

`Docs/RELEASE_GATES.md` already requires JSON reports with commit SHA, OS/kernel, CPU, audio interface/driver, sample rate, period size, duration, xrun count, latency method/results, memory measurements, test command, and pass/fail. This audit recommends retaining that evidence set while adding the prompt's common `harness`, `result`, `machine`, `started_at`, `duration_s`, `metrics`, and human-authored `notes` fields.

## Gap list

1. **ALSA device startup/shutdown:** host CLI device selection, measured startup/shutdown timings, callback period timing, machine metadata, JSON report, and a test seam for ALSA operations are absent.
2. **60-minute zero-xrun:** no strict 60-minute minimum harness that aborts on first xrun and writes a report; no target-machine evidence.
3. **Loopback latency:** no impulse send/capture trial harness or synthetic detector test; no physical/reference loopback evidence.
4. **Underrun injection:** no deterministic injected-transfer-error harness against the production backend; no hardware recovery evidence.
5. **500-track performance:** a synthetic mixer benchmark exists, but no report-path contract, budget enforcement, required machine metadata, or qualifying target-machine result.
6. **8-hour soak:** a synthetic mixer/journal soak exists, but no memory-drift threshold, report-path contract, machine metadata, or qualifying 8-hour result.
7. **Privacy scan:** `tools/` is outside the current source audit's scan set.
8. **CI/manual workflow:** current CI smoke-tests the existing benchmark and soak, but does not test the six requested simulated harness modes, upload their reports, or provide a workflow-dispatch-only target-machine run guide.
9. **Baseline log:** this API-driven editing session did not execute the three local configure/build/CTest command sequences and therefore cannot truthfully populate verbatim output in `Docs/BUILD_LOG.md`. Do not infer a local baseline from a previous CI run.
