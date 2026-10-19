# Build and validation log

## Unlimited / voiceable-track-capacity task — baseline gate

**Status: BLOCKED — baseline commands have not been run.**

The connected repository workflow can inspect and edit GitHub files, but it does not provide a shell in a checked-out local clone. Therefore the commands below were not executed here. No build, CTest, sanitizer, memory measurement, or hardware result is claimed. Per the track-capacity prompt's stop condition, implementation work that depends on a green baseline must not proceed until these commands are run on a machine with the repository and its declared dependencies installed.

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
| Default Release + tests | NOT RUN | No local shell/clone available in this workflow |
| ALSA + tests | NOT RUN | No local shell/clone available in this workflow |
| GUI + ALSA + tests | NOT RUN | No local shell/clone available in this workflow |
| `hush_privacy_source_audit` | NOT RUN for this task baseline | Must be included in the above CTest runs |
| ASan / TSan at the proposed track ceiling | NOT RUN | Must not be inferred from older CI runs |
| Physical audio hardware | NOT RUN | Not authorized to claim from source inspection or CI |

### Stop condition

Do not implement or claim the proposed track-capacity change until all three baseline build-and-CTest configurations are green on the same starting revision. Record exact output, environment, and commit SHA below when actually run. If a command fails, preserve the real diagnostic and fix only baseline blockers before proceeding.
