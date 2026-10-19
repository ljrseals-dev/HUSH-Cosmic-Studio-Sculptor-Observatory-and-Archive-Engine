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

<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1200 450" width="100%" height="100%">
  <defs>
    <style>
      .bg { fill: #0f1117; }
      .warning-card { fill: #1e1e2e; stroke: #f38ba8; stroke-width: 3; rx: 12; }
      .header-title { fill: #f38ba8; font-family: monospace; font-weight: bold; font-size: 28px; letter-spacing: 2px; }
      .sub-text { fill: #cdd6f4; font-family: sans-serif; font-size: 16px; }
      .code-badge { fill: #313244; stroke: #89b4fa; stroke-width: 1.5; rx: 6; }
      .code-text { fill: #a6e3a1; font-family: monospace; font-size: 14px; }
      .accent-line { stroke: #fab387; stroke-width: 3; stroke-dasharray: 6 4; }
    </style>
  </defs>

  <!-- Background -->
  <rect width="1200" height="450" class="bg"/>

  <!-- Warning Banner Card -->
  <rect x="60" y="60" width="1080" height="330" class="warning-card"/>

  <!-- Decorative Accent Line -->
  <line x1="100" y1="130" x2="1100" y2="130" class="accent-line"/>

  <!-- Header Content -->
  <g transform="translate(100, 110)">
    <!-- Warning Icon / Symbol -->
    <path d="M 0,-15 L 15,15 L -15,15 Z" fill="none" stroke="#f38ba8" stroke-width="2"/>
    
    <!-- Title -->
    <text x="35" y="5" class="header-title">DO NOT IMPLEMENT — SCAFFOLDING WARNING</text>
    
    <!-- Subtitle / Directive -->
    <text x="0" y="65" class="sub-text" font-weight="bold" fill="#fab387">STATUS: PROTOTYPE / SCAFFOLDING ONLY — DO NOT DEPLOY OR MERGE TO PRODUCTION</text>
    
    <!-- Description details -->
    <text x="0" y="110" class="sub-text">• This module is part of the initial structural bootstrap for HUSH Cosmic Studio[span_0](start_span)[span_0](end_span).</text>
    <text x="0" y="140" class="sub-text">• Contains placeholder logic, stubs, and non-production-hardened components.</text>
    <text x="0" y="170" class="sub-text">• Pending complete cryptographic, safety, and audio buffer isolation reviews.</text>

    <!-- Code Badge Tag -->
    <g transform="translate(0, 200)">
      <rect width="320" height="36" class="code-badge"/>
      <text x="15" y="23" class="code-text"># TARGET: HUSH-Cosmic-Studio-0.2.1</text>
    </g>
  </g>
</svg>




You're right. This architecture is a bare-bones skeleton. It shows four boxes and a few connections, but it doesn't yet represent a professional, integrated HUSH Cosmic Studio.

The key issue is that the diagram describes components without showing the complete audio signal path, actual engine integration, session model, or the three-panel studio interface.

## What HUSH Cosmic Studio 0.2.1 should show

## HUSH COSMIC STUDIO

Native C++20 · Local-first · Real-time audio architecture

The Sculptor

Timeline · Clips · Mixer · Automation

Observatory

Transport · Spatial view · Master metering

Archive & Engine

Media · Plugins · DSP · Device status

Engine Bridge

Bounded command queue · Parameter state · Telemetry snapshots

### Audio Engine

Audio I/O

Device selection · Input/output buffers

Session & Transport

Tempo · Play/stop · Position

DSP Graph

Tracks · Gain · Pan · Processor chain

Metering & Analysis

Peak · RMS · Clipping · Latency

Audio buffers flow through the active processing graph

Processor chain

Input

Hush DSP

Master Out

The actual Hush Processor must be connected here; the diagram alone does not establish that integration.

## The engineering work missing from the skeleton

| Area           | Required implementation                                  |
| -------------- | -------------------------------------------------------- |
| Session model  | Tracks, clips, routing, tempo, save/load                 |
| Audio I/O      | Device discovery, stream lifecycle, error handling       |
| Engine bridge  | Real commands and telemetry connected to the live engine |
| DSP graph      | Actual processor execution and safe graph updates        |
| Observatory    | Live meters and analysis, not invented display values    |
| UI interaction | Working transport, faders, clip editing, library actions |
| Verification   | Compile, automated tests, GUI launch, audio-device test  |

One important correction: an X11 event loop is a Linux-specific foundation, not a cross-platform GUI strategy. If HUSH is intended for Windows, Linux, and macOS, the windowing and audio-device layers need an explicit portability plan.

I would make the next milestone HUSH Cosmic Studio 0.2.1 — Engine Integration, starting by examining the real `hush-daw` and `hush-processor` source trees, identifying the canonical engine interfaces, and wiring the bridge to those interfaces before claiming the Observatory is live. I haven't inspected the source behind this SVG, so I can't yet say which of these components already exist in code.
You're right. This architecture is a bare-bones skeleton. It shows four boxes and a few connections, but it doesn't yet represent a professional, integrated HUSH Cosmic Studio.

The key issue is that the diagram describes components without showing the complete audio signal path, actual engine integration, session model, or the three-panel studio interface.

## What HUSH Cosmic Studio 0.2.1 should show

## HUSH COSMIC STUDIO

Native C++20 · Local-first · Real-time audio architecture

The Sculptor

Timeline · Clips · Mixer · Automation

Observatory

Transport · Spatial view · Master metering

Archive & Engine

Media · Plugins · DSP · Device status

Engine Bridge

Bounded command queue · Parameter state · Telemetry snapshots

### Audio Engine

Audio I/O

Device selection · Input/output buffers

Session & Transport

Tempo · Play/stop · Position

DSP Graph

Tracks · Gain · Pan · Processor chain

Metering & Analysis

Peak · RMS · Clipping · Latency

Audio buffers flow through the active processing graph

Processor chain

Input

Hush DSP

Master Out

The actual Hush Processor must be connected here; the diagram alone does not establish that integration.

## The engineering work missing from the skeleton

| Area           | Required implementation                                  |
| -------------- | -------------------------------------------------------- |
| Session model  | Tracks, clips, routing, tempo, save/load                 |
| Audio I/O      | Device discovery, stream lifecycle, error handling       |
| Engine bridge  | Real commands and telemetry connected to the live engine |
| DSP graph      | Actual processor execution and safe graph updates        |
| Observatory    | Live meters and analysis, not invented display values    |
| UI interaction | Working transport, faders, clip editing, library actions |
| Verification   | Compile, automated tests, GUI launch, audio-device test  |

One important correction: an X11 event loop is a Linux-specific foundation, not a cross-platform GUI strategy. If HUSH is intended for Windows, Linux, and macOS, the windowing and audio-device layers need an explicit portability plan.

I would make the next milestone HUSH Cosmic Studio 0.2.1 — Engine Integration, starting by examining the real `hush-daw` and `hush-processor` source trees, identifying the canonical engine interfaces, and wiring the bridge to those interfaces before claiming the Observatory is live. I haven't inspected the source behind this SVG, so I can't yet say which of these components already exist in code.

