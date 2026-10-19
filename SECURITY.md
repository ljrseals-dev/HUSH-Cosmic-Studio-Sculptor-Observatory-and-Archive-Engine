# Security Policy

## Privacy and network behavior

HUSH is designed for zero telemetry: no analytics, telemetry events, crash uploads, update checks, or remote configuration. The build does not bootstrap dependencies or fetch source code. The runtime engine is intended to operate without network access. Optional system dependencies must be installed before configuration.

These guarantees describe the current project design and code paths; they are not a substitute for independent security review of every future change. Report any unexpected network activity or privacy-sensitive behavior as a security issue.

## Reporting a vulnerability

Please use GitHub's private vulnerability reporting feature for this repository if it is enabled. If private reporting is unavailable, contact the repository maintainer privately through a verified channel before publishing exploit details. Include affected commit/version, reproduction steps, impact, and logs with secrets or personal data removed. Do not attach credentials, private audio, or other sensitive material to a public issue.

## Scope

Security reports may include unexpected network access, unsafe project-file parsing, memory-safety issues, audio-thread violations, data loss, and dependency/build supply-chain concerns.
