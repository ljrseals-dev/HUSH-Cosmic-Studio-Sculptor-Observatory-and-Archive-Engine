# HUSH 0.3a — research milestone

This branch is deliberately isolated from `main`: `research/0.3a-full-milestone`.

## Scope implemented in this research slice

- **Arrangement model:** sample-frame clip placement, move/resize operations, duration calculation, duplicate-ID validation, and transactional replacement.
- **Plugin editor model:** validated parameter descriptors and bounded parameter-value state. This is the model a native plugin editor can bind to; it is not a VST3/LV2 plugin host and does not load third-party binaries.
- **MIDI input core:** allocation-free MIDI 1.0 channel-message byte parser with running status, note-on/off, CC, pitch bend, and program change. System-common/SysEx and hardware device enumeration are not included.
- **Recording capture boundary:** fixed-size stereo blocks enter a bounded SPSC queue from an audio callback without allocation, locks, waiting, or file I/O. A recording worker must drain the queue and write the WAV file; overflow is counted rather than blocking audio.
- **Voice command boundary:** deterministic, explicit text-command parser for play, stop, record, mute-track, and set-tempo. This is not speech recognition and does not access a microphone.
- **Contract tests:** cover validation, failure atomicity, MIDI running status, command parsing, and recorder-queue overflow.

## Safety and integration boundaries

No feature silently opens a device, starts recording, activates a plugin, captures a microphone, or executes arbitrary commands. The caller owns confirmation, permissions, lifecycle, and routing. Plugin hosting, Qt arrangement drawing, actual MIDI backend enumeration, a background recording worker/file lifecycle, speech-to-text, and full live-host integration remain follow-on integration work. Do not describe this research slice as a complete production DAW milestone until those boundaries are implemented and verified.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DHUSH_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Build/test commands have not been run by this commit authoring action. Results must come from CI or a local build.
