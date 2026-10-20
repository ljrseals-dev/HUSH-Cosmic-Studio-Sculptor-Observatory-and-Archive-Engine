# Build and validation log

## Unlimited / voiceable-track-capacity task — baseline gate

**Status: CI BASELINE GREEN; local command sequence not run.**

The exact shell sequence was not run in a local checkout in this workflow. However, GitHub Actions run [39449869574](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39449869574) passed on the same starting commit SHA `d4a9ea3e443937a84b2d882c0629468f996874f1`. Its jobs ran the default Release configure/build/CTest, ALSA configure/build/CTest, Qt GUI configure/build/CTest, combined Qt+ALSA configure/build/CTest, ASan contracts, TSan contracts, and repository artifact hygiene. This satisfies the baseline-green decision for proceeding with the feature branch, but it is not a claim that the exact combined local command sequence was run. No 1024-track allocation-guard, memory, performance, soak, or hardware result is claimed.

### Required baseline commands (verbatim)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DHUSH_BUILD_TESTS=ON
cmake --build build && ctest --test-dir build --output-on-failure
cmake -S . -B build-alsa -DHUSH_ENABLE_ALSA=ON -DHUSH_BUILD_TESTS=ON
cmake --build build-alsa && ctest --test-dir build-alsa --output-on-failure
cmake -S . -B build-gui -DHUSH_BUILD_TESTS=ON -DHUSH_BUILD_GUI=ON -DHUSH_ENABLE_ALSA=ON
cmake --build build-gui --parallel 2 && ctest --test-dir build-gui --output-on-failure
```

### Results

| Configuration | Result | Evidence |
|---|---|---|
| Default Release + tests | PASS in CI | [run 39449869574](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39449869574), job `linux-cxx20`, SHA `d4a9ea3e443937a84b2d882c0629468f996874f1` |
| ALSA + tests | PASS in CI | Same run, job `linux-alsa-enabled`, same SHA |
| GUI + ALSA + tests | PASS in CI | Same run, job `linux-qt6-alsa-live-mixer`, same SHA; GUI-only also passed in `linux-qt6-mixer-gui` |
| `hush_privacy_source_audit` | PASS in CI baseline | Included in the baseline CTest jobs above; rerun after feature changes |
| ASan / TSan at the proposed track ceiling | NOT RUN | Must not be inferred from older CI runs |
| Physical audio hardware | NOT RUN | Not authorized to claim from source inspection or CI |

### Stop condition

Baseline CI is green on one starting revision, so feature work may proceed on a separate branch. Re-run all configurations on the feature head and record their real results before declaring completion. If a command fails, preserve the real diagnostic and fix only baseline blockers before proceeding.
