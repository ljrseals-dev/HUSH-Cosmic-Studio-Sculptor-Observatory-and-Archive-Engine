# HUSH 0.6a — VST3 hosting entry gate

## Scope

This is the first engineering gate for Prompt 6 in the unified prompt sequence (Docs/Prompt): VST3 hosting on Linux, with Windows preparation only. CLAP and AU are out of scope. This document is a repository inspection and dependency gate; it does not claim that a VST3 host has been implemented.

## Inspected source state

- The current main tree contains no VST3 SDK checkout, VST3 host target, plugin-host process, plugin IPC implementation, plugin scanner, plugin instance/state model, or plugin-host contract tests.
- The project is currently C++20/CMake. Normal builds must remain offline and must not download, bootstrap, or fetch the SDK.
- The project schema is v1. Prompt 6 requires a schema-v2 migration when real insert/plugin state is introduced; schema must not be bumped merely to reserve empty fields.
- Existing real-time callback contracts forbid allocation, locks, logging, disk I/O, and network access. Plugin loading/scanning and IPC setup therefore belong off the audio thread.
- The privacy contract requires no telemetry, call-home behavior, automatic network requests, or hidden capture.

## Required first decision: SDK availability

The repository does not contain the Steinberg VST3 SDK and this workflow has no checked-out local environment in which to verify a preinstalled SDK. Do not add FetchContent, a submodule that fetches during build, download scripts, or an automatic bootstrap to make the build pass.

Before implementing the SDK adapter, provide a locally installed/licensed SDK directory to the build environment or add a reviewed SDK source archive to the repository with its license and provenance recorded. Then make SDK integration an explicit opt-in CMake feature and verify that the default build remains SDK-independent and offline.

## Architecture decisions required by Prompt 6

1. **Process isolation default:** plugin processing runs in a separate host process by default. Trusted in-process hosting, if supported later, must be explicit opt-in and documented as a stability/latency trade-off.
2. **IPC contract:** define versioned, bounded messages for instance creation/destruction, audio block transfer, parameter changes, state save/restore, latency reports, and structured failure. Reject malformed/oversized messages; do not let IPC block the audio callback.
3. **Audio transport:** use preallocated shared memory/ring buffers or another explicitly bounded mechanism; no per-block allocation or locks in the engine callback. Document scheduling, ownership, back-pressure, and failure behavior.
4. **Crash/hang containment:** the engine must not dereference sandbox-owned memory after process death. Watchdog/restart work stays off the callback and reports a structured error to the UI/control layer.
5. **State and schema:** plugin identity, instance UUID, parameter/state payload, bypass state, and reported latency must be persisted with a tested schema-v1 to v2 migration. Plugin state blobs need explicit size limits and local-only storage rules.
6. **Scanning:** run plugin scanning in a sandbox with a watchdog. Never scan plugins during audio processing or silently execute arbitrary binaries during project open.
7. **Latency:** report plugin latency and compensate only after a tested graph-level delay model exists. Do not claim compensation merely because a plugin reports a number.
8. **Insert points:** activate existing insert stubs only through a defined track/bus/master processing order with deterministic bypass and failure semantics.

## Stop conditions

Stop before SDK-dependent source implementation if the SDK is unavailable, its license/provenance cannot be established, or the proposed IPC/audio transport needs callback allocation, blocking locks, file I/O, or network access. Record the concrete blocker and request the missing local dependency; do not replace the SDK with a fake plugin API or claim a plugin is loaded.

## Validation still required

- Default offline configure/build/CTest, unchanged without the SDK.
- SDK-enabled configure/build/CTest using the locally supplied SDK.
- Plugin-host process and IPC contract tests, including malformed messages, process crash, watchdog timeout, and shutdown with active audio.
- State save/restore and schema-v1 migration tests.
- ASan/TSan CI for the host/IPC lifecycle.
- The Prompt 6 50-plugin / 20-track / 60-minute zero-xrun gate remains open until run on a named target machine with evidence. No hardware, performance, or soak result is claimed by this gate.
