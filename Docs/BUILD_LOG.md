# Build log — Gate 7 baseline

## Evidence source and limitation

This environment can inspect and edit the GitHub repository and read GitHub Actions job outcomes, but it does not provide a terminal attached to a local clone. Therefore the three requested shell commands were **not run locally**, and their raw terminal output is not available. This log deliberately does not fabricate verbatim CMake/CTest output. The equivalent CI configurations did pass on the baseline revision below.

Baseline CI run: [GitHub Actions run 39255426377](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39255426377), revision before Gate 7 changes.

Observed job step outcomes (exact GitHub Actions step names and conclusions):

- `linux-cxx20`: “Configure offline build” — success; “Build” — success; “Contract tests” — success.
- `linux-alsa-enabled`: “Configure ALSA-enabled build” — success; “Build ALSA backend and realtime host” — success; “Contract tests” — success.
- `linux-qt6-alsa-live-mixer`: “Configure combined live mixer GUI build” — success; “Build GUI and ALSA live monitor” — success; “Contract tests” — success.
- `linux-asan`: “Configure AddressSanitizer build” — success; “Build” — success; “Contract tests” — success.
- `linux-tsan`: “Configure ThreadSanitizer build” — success; “Build” — success; “Contract tests” — success.
- `linux-qt6-mixer-gui`: “Configure GUI build” — success; “Build GUI and contracts” — success; “Contract tests” — success.
- `repo-artifact-hygiene`: “Reject committed build outputs and new binary archives” — success.

Requested local commands, not executed here:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DHUSH_BUILD_TESTS=ON
cmake --build build && ctest --test-dir build --output-on-failure
cmake -S . -B build-alsa -DHUSH_ENABLE_ALSA=ON -DHUSH_BUILD_TESTS=ON
cmake --build build-alsa && ctest --test-dir build-alsa --output-on-failure
cmake -S . -B build-gui -DHUSH_BUILD_TESTS=ON -DHUSH_BUILD_GUI=ON -DHUSH_ENABLE_ALSA=ON
cmake --build build-gui --parallel 2 && ctest --test-dir build-gui --output-on-failure
```

Do not treat the baseline CI results as target-machine hardware validation, power-loss testing, or proof of zero network activity. Append later CI runs and any owner-provided local transcripts as separate dated entries; preserve their output verbatim when actually available.

## Gate 7 revision check — 2026-10-19

- Review branch: `feat/autosave-lifecycle-gate7`, PR [#36](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/36).
- Latest code commit observed before this log update: `4a5e4511369116d5aad2edcc9c8d30f238e6fb2b`.
- No workflow run for this branch/head was returned by the GitHub Actions run listing at the time of this entry. The baseline run above is **not** evidence that the Gate 7 revision compiles. Do not mark Gate 7 build/test checks green until a fresh run or local transcript is available.
- Local clone build remains unavailable in this execution environment because `git clone https://github.com/...` failed with `Could not resolve host: github.com`. This is an environment/network failure, not a CMake failure; no build was attempted after clone failed.
