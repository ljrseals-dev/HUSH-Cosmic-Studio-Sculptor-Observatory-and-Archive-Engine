# Build and validation log

## Unlimited / voiceable-track-capacity task — baseline gate

**Baseline CI: PASS on starting revision `d4a9ea3e443937a84b2d882c0629468f996874f1`.** GitHub Actions run [39449869574](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39449869574) completed successfully on 2026-10-20. This is CI evidence, not a local shell run. No output has been fabricated; the workflow summary records successful configure/build/CTest steps.

### Required commands (verbatim from the task prompt)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DHUSH_BUILD_TESTS=ON
cmake --build build && ctest --test-dir build --output-on-failure
cmake -S . -B build-alsa -DHUSH_ENABLE_ALSA=ON -DHUSH_BUILD_TESTS=ON
cmake --build build-alsa && ctest --test-dir build-alsa --output-on-failure
cmake -S . -B build-gui -DHUSH_BUILD_TESTS=ON -DHUSH_BUILD_GUI=ON -DHUSH_ENABLE_ALSA=ON
cmake --build build-gui --parallel 2 && ctest --test-dir build-gui --output-on-failure
```

### Baseline configuration results

| Configuration | Result | Evidence |
|---|---|---|
| Default Release + tests | PASS | CI job `linux-cxx20`, run 39449869574 |
| ALSA + tests | PASS | CI job `linux-alsa-enabled`, run 39449869574 |
| GUI + ALSA + tests | PASS | CI job `linux-qt6-alsa-live-mixer`, run 39449869574 |
| Qt GUI-only tests | PASS | CI job `linux-qt6-mixer-gui`, run 39449869574 |
| AddressSanitizer baseline | PASS | CI job `linux-asan`, run 39449869574 |
| ThreadSanitizer baseline | PASS | CI job `linux-tsan`, run 39449869574 |
| `hush_privacy_source_audit` | PASS as part of CTest | Included in the successful CTest jobs above |
| ASan / TSan with 1,024 tracks and allocation guard | NOT RUN | Capacity implementation and full-capacity guard do not yet exist |
| Physical audio hardware | NOT RUN | CI is not hardware validation |

The sanitizer jobs validate the current baseline tests only. They do **not** establish full-capacity track correctness, a no-allocation guarantee under an allocation guard, long-duration stability, or hardware behavior.

### Next gate

The baseline prerequisite is satisfied by the same starting revision in CI. Proceed with the capacity implementation in separate logical commits. After the code changes, rerun all configurations and sanitizers on the new PR head; do not use this baseline run as evidence for code that did not exist at this SHA.
