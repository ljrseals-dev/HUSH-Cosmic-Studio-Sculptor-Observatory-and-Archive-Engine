# HUSH Cosmic Studio — Source Handoff 0.2.1

A small, offline-first C++20 integration handoff for the HUSH Cosmic Studio concept: **Sculptor**, **Observatory**, and **Archive & Engine**.

## What is included
- CMake-based C++20 core and smoke test.
- `AudioEngine` owns an `EngineBridge`; the process path forwards the buffer through the bridge.
- Minimal Linux/X11 three-panel UI shell (optional).
- Demonstration native module ABI and example gain module.
- Security, dependency, and implementation-status notes.

## Build core and run test
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Optional X11 shell
On Linux with X11 development headers installed:
```sh
cmake -S . -B build-ui -DHUSH_BUILD_X11_UI=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-ui --parallel
./build-ui/hush_ui
```
Press any key while the UI window is focused to close it.

## Scope and release honesty
This is a **source handoff/starter**, not a completed production DAW. The example module ABI is HUSH-native and is **not VST3 or LV2**. No real audio-device playback, recording, full arrangement/session editing, production plugin hosting, or live connection to the separate `hush-processor` repository is claimed here. Hardware testing and security audit have not been performed. See `docs/IMPLEMENTATION_STATUS.md`.

No telemetry, analytics, crash upload, update checking, remote configuration, or network bootstrap is included.
