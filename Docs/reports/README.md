# Target-machine validation reports

These JSON files are **templates only**. A report may be replaced with measured output only after a human runs the matching harness on the named target machine and reviews the raw output. Never commit guessed or interpolated measurements.

## Common schema

- `harness`: stable harness name.
- `result`: `PASS`, `FAIL`, or `NOT_RUN`.
- `machine`: machine identity including uname/kernel, CPU, ALSA version, and named device where relevant.
- `started_at`: ISO-8601 timestamp recorded by the harness.
- `duration_s`: measured elapsed duration; use `null` in an unrun template.
- `metrics`: measurements emitted by the harness.
- `notes`: human-written caveats, wiring/configuration details, and interpretation.

The report should additionally record the commit SHA, exact command, sample rate, period size, test method, and relevant threshold. Simulated CI runs test harness logic only; they are not hardware evidence and must not overwrite these human-run report templates.

## Hardware evidence policy

A committed measured report must be produced by a human running the script on a named machine and pasting/reviewing the actual output. Include machine identity, timestamp, measured duration, and human-written notes. Keep `result: NOT_RUN` until that evidence exists. These reports do not close Gates 4–6 by themselves; each gate remains open until qualifying evidence is reviewed.
