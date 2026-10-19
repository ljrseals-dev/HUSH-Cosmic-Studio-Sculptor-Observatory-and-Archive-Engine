# Pull-request conflict inventory and resolution log

## 2026-10-19 — open PR inventory

Inventory source: GitHub REST open-pull-request list and per-PR metadata for this repository. PR mergeability is computed by GitHub and can be `unknown` until recalculated. A `dirty` merge state means the branch is reported as conflicting with its base; it does **not** expose exact conflict markers or a trustworthy per-file unmerged list.

| PR | Title | Head | Base | Reported merge state | Action |
|---|---|---|---|---|---|
| [#36](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/36) | Close Gate 7: autosave lifecycle and failure surfacing | `feat/autosave-lifecycle-gate7` | `main` | `dirty` | Needs local merge diagnosis before resolution |
| [#33](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/33) | test: add headless Qt mixer interaction coverage | `test/gui-interaction-gate1` | `main` | `dirty` | Needs local merge diagnosis before resolution |
| [#32](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/32) | Test GUI-to-engine mixer command delivery | `test/gui-engine-command-path-contract-clean` | `main` | `dirty` | Needs local merge diagnosis before resolution |
| [#27](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/27) | automation: add sample-positioned lane interpolation foundation | `phase1-automation-foundation` | `main` | `dirty` | Needs local merge diagnosis; automation scope must remain aligned with current master prompt |
| [#24](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/24) | Propose transport, MIDI learn, and automation design | `docs/transport-midi-automation-proposal` | `main` | `dirty` | Docs-only proposal; do not imply proposed features are implemented |
| [#17](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/17) | Add control-side mixer gain bridge and contract tests | `milestone/0.3c-mixer-command-bridge` | `main` | `unknown` | Re-query mergeability; no conflict conclusion |
| [#23](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/23) | Add initial transport, tempo-map, and automation core | `work/phase0-phase1-automation-start` | `main` | `unknown` | Re-query mergeability; inspect scope before integrating |
| [#26](https://github.com/ljrseals-dev/HUSH-Cosmic-Studio-Sculptor-Observatory-and-Archive-Engine/pull/26) | transport: add initial transport and automation foundations | `codex/phase0-transport-automation-proposal` | `main` | `unknown` | Re-query mergeability; inspect scope before integrating |

PR #22 (privacy source-contract PR) is **closed**, not an open conflicting PR. No claim is made here about whether it was merged; its close state alone does not establish that.

## What is and is not diagnosed

The GitHub PR file listings show overlapping edits among the dirty PRs, including `CMakeLists.txt`, `README.md`, `CHANGELOG.md`, `ROADMAP.md`, `Docs/IMPLEMENTATION_STATUS.md`, `Docs/HUSH_0.3A_MIXER_GUI.md`, `Docs/PHASE_1_TO_7_EXECUTION.md`, `include/hush/realtime_graph_publisher.hpp`, `include/hush/published_realtime_bus_engine.hpp`, `src/mixer_gui.cpp`, and GUI command-path tests. These are **candidate overlap files**, not an assertion that each is an actual Git conflict.

The available GitHub contents/PR metadata API does not provide the actual local `git status`, conflict markers, or `git diff --name-only --diff-filter=U` output. No PR branch was rewritten or force-pushed. Exact semantic conflict diagnosis is therefore pending an authenticated local clone.

## Exact local commands to complete diagnosis safely

Run from a clean authenticated clone. Preserve local work; do not reset or force-push.

```sh
gh pr list --state open --json number,title,headRefName,baseRefName,mergeable,mergeStateStatus
git config merge.conflictstyle zdiff3
gh pr checkout 24
git fetch origin main
git status --short
git merge origin/main --no-commit
git status
git diff --name-only --diff-filter=U
```

Record the actual unmerged paths and semantic intent before editing. Resolve only after reading both sides and the relevant privacy/realtime/schema constraints. Then run the required default, ALSA-enabled, and GUI+ALSA builds plus CTest. If a conflict requires a schema/privacy owner decision, leave that PR unresolved and document the alternatives. Repeat diagnosis for each reported dirty PR in a separate clean worktree or after safely aborting the prior merge.

If `gh` is unavailable or unauthenticated, stop before any push. Do not manufacture conflict-marker evidence from a PR file list.


## Follow-up reconciliation — 2026-10-19

The original inventory above is a historical snapshot and must not be read as current branch state. The shared build log has since landed on `main` in PR #47, so this branch has been rebased onto current `main` and no longer carries a competing copy of `Docs/BUILD_LOG.md`.

Current relevant PR checks at this reconciliation:
- PR #37 (this documentation PR): rebase applied on current `main`; only `Docs/PR_CONFLICT_LOG.md` remains as its proposed change.
- PR #46 (track-capacity audit): rebased onto current `main`; its proposed change is limited to `Docs/UNLIMITED_TRACKS_AUDIT.md`. The baseline gate remains blocked until the owner-run default, ALSA, and GUI+ALSA configure/build/CTest commands are green.
- PR #45 (sample-positioned clip model): remains a separate code integration candidate touching `CMakeLists.txt`, `CHANGELOG.md`, `ROADMAP.md`, `include/hush/project.hpp`, and `src/project.cpp`. These overlapping code files require a fresh compare and CI before integration; do not resolve by blindly copying the entire branch over current `main`.
- PR #42 (WAV export foundation) is merged. PR #47 (track-capacity baseline log) is merged.

The repository connector can compare refs and update tracked files, but it does not provide an authenticated local checkout or terminal merge session. Therefore no claim is made that local `git diff --name-only --diff-filter=U` was run. Mergeability metadata can lag after a ref update; use the current base/head compare and the PR's refreshed checks rather than treating an old `mergeable=false` field as definitive by itself.

## Safe resolution policy

1. Rebase documentation-only branches by applying their unique document to the current base, not by reintroducing stale copies of shared files.
2. For code branches, inspect each changed file and port the intended hunks onto current `main`; do not replace current files wholesale where both branches changed them.
3. Run the three required build/CTest configurations and privacy audit in a local checkout before marking code conflicts resolved or merging.
4. Keep the track-capacity implementation stopped until the required baseline is green, per the owner prompt.
