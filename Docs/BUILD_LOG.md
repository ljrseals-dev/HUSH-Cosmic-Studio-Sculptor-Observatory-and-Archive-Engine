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
- Fresh CI for the source/test revision passed all seven jobs in [run 39291973496](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39291973496); see the latest entry below. The subsequent commit changes documentation only.
- Do not mark Gate 7 build/test checks green until a fresh run or an owner-provided local transcript is available.
- The privacy source audit is registered in CTest but has not been rerun for this branch.
- Power-loss durability, target-device validation, long soak stability, and zero-network runtime verification are not claimed.

## Gate 7 CI result — commit `20736229f71da41e6fe7cc58cd4a84885809a2db`

GitHub Actions [run 39291973496](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39291973496) completed with all seven jobs `success`:

- `linux-cxx20`: “Configure offline build”, “Build”, “Contract tests”, and “Smoke-test benchmark and recovery soak harnesses” — success.
- `linux-alsa-enabled`: “Configure ALSA-enabled build”, “Build ALSA backend and realtime host”, and “Contract tests” — success.
- `linux-qt6-mixer-gui`: “Configure GUI build”, “Build GUI and contracts”, and “Contract tests” — success.
- `linux-qt6-alsa-live-mixer`: “Configure combined live mixer GUI build”, “Build GUI and ALSA live monitor”, and “Contract tests” — success.
- `linux-asan`: “Configure AddressSanitizer build”, “Build”, and “Contract tests” — success.
- `linux-tsan`: “Configure ThreadSanitizer build”, “Build”, and “Contract tests” — success.
- `repo-artifact-hygiene`: “Reject committed build outputs and new binary archives” — success.

This validates the code at the listed commit. The source, test, and CMake revision at commit `30d58d6f49b1854a6d0bbc8b4375ee98f8ffeff5` is green in run 39291973496. This subsequent change is documentation-only; the tested source/build/test files are unchanged. The local CMake commands above remain unrun here because GitHub clone access failed DNS resolution; no local output has been fabricated.

## Latest Gate 7 source/build/test revision — commit `30d58d6f49b1854a6d0bbc8b4375ee98f8ffeff5`

GitHub Actions [run 39291973496](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39291973496) completed with all seven jobs `success`:

- `linux-cxx20`: configure offline build, build, contract tests, benchmark/recovery-soak smoke tests — success.
- `linux-alsa-enabled`: configure ALSA build, build ALSA backend and host, contract tests — success.
- `linux-qt6-mixer-gui`: configure GUI build, build GUI/contracts, contract tests — success.
- `linux-qt6-alsa-live-mixer`: configure combined GUI+ALSA build, build GUI/ALSA monitor, contract tests — success.
- `linux-asan`: configure, build, contract tests — success.
- `linux-tsan`: configure, build, contract tests — success.
- `repo-artifact-hygiene`: archive/build-output hygiene check — success.

This includes the privacy source audit registered in the default contract-test suite. It does not cover a real ALSA device, Qt dialog automation, CLI dirty-quit failure, power-loss durability, or long-duration hardware tests.
