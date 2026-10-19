# Build log — Gate 7 baseline

## Baseline evidence before Gate 7 changes

The baseline source revision is covered by [GitHub Actions run 39273263409](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39273263409). The following exact job steps reported `success`:

- `linux-cxx20`: “Configure offline build”; “Build”; “Contract tests”; “Smoke-test benchmark and recovery soak harnesses”.
- `linux-alsa-enabled`: “Configure ALSA-enabled build”; “Build ALSA backend and realtime host”; “Contract tests”.
- `linux-qt6-mixer-gui`: “Configure GUI build”; “Build GUI and contracts”; “Contract tests”.
- `linux-qt6-alsa-live-mixer`: “Configure combined live mixer GUI build”; “Build GUI and ALSA live monitor”; “Contract tests”.
- `linux-asan`: “Configure AddressSanitizer build”; “Build”; “Contract tests”.
- `linux-tsan`: “Configure ThreadSanitizer build”; “Build”; “Contract tests”.
- `repo-artifact-hygiene`: “Reject committed build outputs and new binary archives”.

This is CI evidence for the baseline revision only, not the Gate 7 branch or target-machine validation.

## Requested local commands — not executed in this environment

This environment does not provide a terminal attached to a local clone. Attempting `git clone` failed with `fatal: unable to access 'https://github.com/...': Could not resolve host: github.com`. No local CMake command was run, so no local stdout/stderr transcript exists. The text below records the requested commands verbatim; it is not a claim that they were executed here.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DHUSH_BUILD_TESTS=ON
cmake --build build && ctest --test-dir build --output-on-failure
cmake -S . -B build-alsa -DHUSH_ENABLE_ALSA=ON -DHUSH_BUILD_TESTS=ON
cmake --build build-alsa && ctest --test-dir build-alsa --output-on-failure
cmake -S . -B build-gui -DHUSH_BUILD_TESTS=ON -DHUSH_BUILD_GUI=ON -DHUSH_ENABLE_ALSA=ON
cmake --build build-gui --parallel 2 && ctest --test-dir build-gui --output-on-failure
```

## Gate 7 status

- Branch: `feat/autosave-lifecycle-gate7-main`.
- Fresh CI for this branch has not reported at the time of this entry.
- Do not mark Gate 7 build/test checks green until a fresh run or an owner-provided local transcript is available.
- The privacy source audit is registered in CTest but has not been rerun for this branch.
- Power-loss durability, target-device validation, long soak stability, and zero-network runtime verification are not claimed.
