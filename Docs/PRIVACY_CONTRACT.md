# HUSH privacy contract

HUSH is designed to be a local, offline-first studio. Privacy is a release requirement, not a marketing inference.

## Required behavior

- **No telemetry or analytics:** no usage events, behavioral analytics, tracking identifiers, or telemetry SDKs.
- **No outbound call-home traffic:** no update checks, remote configuration, license pings, crash uploads, crash-reporting services, remote logging, or automatic network requests.
- **No hidden audio capture:** audio is processed only through the explicitly selected local audio device/session. No background capture or transmission.
- **Local files stay local:** sessions, audio, exports, recovery journals, and reports are written only to user-selected or documented local paths. No automatic upload or cloud sync.
- **Explicit user action:** opening, saving, exporting, scanning, and recovery are initiated by the user or by a clearly documented local session policy. No silent external side effects.
- **Realtime safety:** audio-device callbacks are allowed and necessary for audio I/O. They are not call-home callbacks. Device callbacks must not allocate, block on locks, log, access the network, or perform disk I/O.
- **No callback registration to third parties:** the application must not register outbound webhooks, remote event sinks, analytics hooks, or crash-upload handlers.
- **Optional components are opt-in:** optional backends and utilities remain off unless explicitly enabled; their dependencies must be preinstalled. No configure-time dependency downloads or bootstrap scripts.

## Build and review rules

1. Default builds must work without network access when declared system dependencies are already installed.
2. Runtime source must not introduce HTTP clients, telemetry SDKs, remote crash reporters, update checkers, or remote configuration.
3. Any future feature that needs network access is out of scope for the default DAW and requires a separately reviewed, explicitly opt-in design. It must not run in the audio thread.
4. The `hush_privacy_source_audit` CTest checks a small denylist of known outbound-network and telemetry APIs in `src/` and `include/`. It is a regression tripwire, **not** a complete static analysis or proof of zero network activity. Review dependencies and runtime behavior separately.
5. Release evidence must include dependency inventory, source audit results, and a runtime network-observation test on the target build. Do not claim a runtime network test unless it was actually performed.

## Scope boundary

This contract does not prohibit local Qt signal/slot connections or the ALSA audio processing callback. Those are in-process/device mechanisms. It prohibits telemetry, hidden callbacks to remote services, and network call-home behavior.
