# Build and validation log

## Unlimited / voiceable-track-capacity task

### Baseline before code changes

Starting revision `d4a9ea3e443937a84b2d882c0629468f996874f1` passed the existing CI configurations in [run 39449869574](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39449869574): default C++20, ALSA, Qt GUI, Qt+ALSA, ASan, TSan, and repository artifact hygiene. This was CI evidence, not a local shell run.

Required baseline commands from the prompt:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DHUSH_BUILD_TESTS=ON
cmake --build build && ctest --test-dir build --output-on-failure
cmake -S . -B build-alsa -DHUSH_ENABLE_ALSA=ON -DHUSH_BUILD_TESTS=ON
cmake --build build-alsa && ctest --test-dir build-alsa --output-on-failure
cmake -S . -B build-gui -DHUSH_BUILD_TESTS=ON -DHUSH_BUILD_GUI=ON -DHUSH_ENABLE_ALSA=ON
cmake --build build-gui --parallel 2 && ctest --test-dir build-gui --output-on-failure
```

### Track-ceiling implementation CI

PR head `51f89e972caa0b88df14fcae81de7dfa4bf9ac17` passed all seven jobs in [CI run 39451685619](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/actions/runs/39451685619) on 2026-10-20:

| Configuration | Result | Evidence |
|---|---|---|
| Default C++20 build + CTest | PASS | `linux-cxx20`, run 39451685619 |
| ALSA build + CTest | PASS | `linux-alsa-enabled`, run 39451685619 |
| Qt GUI build + CTest | PASS | `linux-qt6-mixer-gui`, run 39451685619 |
| Qt + ALSA build + CTest | PASS | `linux-qt6-alsa-live-mixer`, run 39451685619 |
| AddressSanitizer build + CTest | PASS | `linux-asan`, run 39451685619 |
| ThreadSanitizer build + CTest | PASS | `linux-tsan`, run 39451685619 |
| Repository artifact hygiene | PASS | `repo-artifact-hygiene`, run 39451685619 |
| `hush_privacy_source_audit` | PASS as part of CTest | Included in successful test jobs |

The new tests validate schema-v1 acceptance at `HUSH_MAX_TRACKS`, rejection at ceiling+1, rejection by realtime mixer configuration above the ceiling, and no calls to global `operator new` during a 1,024-track / 128-frame `RealtimeTrackMixer::process()` call after setup. These tests passed in the ASan and TSan CI jobs. The allocation guard covers C++ `operator new` calls in that mixer process test; it is not a universal proof against every possible allocator/API or every audio path.

### Not run / still open

- No local configure/build/CTest run was performed in this workflow; results above are GitHub Actions evidence.
- No memory/RSS measurement was run.
- No 1,024-track graph-swap stress test was added yet.
- No 500-track performance qualification or 8-hour soak was run.
- No physical audio hardware validation was performed.
- GUI Add Track / Remove Last Track controls, undoable structural operations, count indicator, and block-boundary active-count commands have been added on the current PR branch. Fresh CI for these later changes is pending. Headless Qt widget-interaction tests and CLI parity remain open.
