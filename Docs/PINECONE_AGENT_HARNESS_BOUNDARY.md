# Pinecone Neural — HUSH Research Harness Review

## Scope and decision

Use `ljrseals-dev/pinecone-neural` as an optional, out-of-process research and review harness. It is not a HUSH runtime dependency and is not connected to the audio engine, Qt host, ALSA backend, project loader, or build system.

- Repository: https://github.com/ljrseals-dev/pinecone-neural
- Reviewed branch: `main`
- Reviewed commit/tree: `8260ee84fc6deb96aeebe46079d47948a30cd84d`
- Review method: static source inspection through the repository files; the agent service itself was not launched.
- HUSH baseline: `main` at `8e4b16af1e4fd9bd18f7b2c1c55e664b8dbfaa43`.
- This review does not claim that a custom HTTP `User-Agent: ljrseals-dev/pinecone-neural` was sent; the available GitHub connector does not expose a custom-header parameter.

## What Pinecone Neural actually provides

The repository is a Python/FastAPI agent harness with OpenAI model calls, Pinecone vector memory, a structured tool registry, API-key authentication, session stores, and optional local SQLite neural-memory components (GNN reranking, synaptic associations and utility scoring).

It is not a GitHub-native autonomous coding agent. Its README describes restricted tool execution, but the checked-in source contains both a typed registry and a separate legacy arbitrary-code path. The registry and sandbox module are not fully wired into the active agent loop.

## Source findings that must be resolved before treating the harness as trusted

These are static code findings, not claims of exploitation.

1. **Legacy arbitrary Python execution remains active.** `app/harness.py::execute_restricted` still uses Python `eval` and falls back to `exec`. The presence of `app/tools/registry.py` and `app/tools/sandbox_exec.py` does not make this safe because the active loop calls `execute_restricted`, not the registry or gVisor sandbox.
2. **The gVisor sandbox is not the active tool path.** `sandbox_exec.py` invokes Docker with `runsc`, network disabled, memory/CPU/PID limits and a timeout, but `app/harness.py` does not call it. The Docker image is a mutable tag, not pinned by digest. The timeout handler kills the process but does not explicitly await process termination after kill.
3. **Neural write signature mismatch.** `PineconeMemory.remember` calls `self.neural.on_remember(vid, emb, importance=importance)`, while `NeuralRecallPipeline.on_remember` requires `(memory_id, vector, text, metadata)`. The exception is swallowed as a warning, so the vector sidecar and synaptic node write are skipped.
4. **Neural recall signature mismatch.** `PineconeMemory.recall` calls `self.neural.enhance(out, top_k=top_k)`, while `NeuralRecallPipeline.enhance` requires `(query_vec, matches, top_k)`. The exception is caught and raw ANN results are returned; the GNN/synaptic enhancement path therefore does not run through this caller as written.
5. **The memory API and tool registry are not actually joined.** `app/main.py` builds a registry and assigns it only if the harness has a `tools` attribute, but the shown `AgentHarness` tool loop continues to parse model-provided Python code and call `execute_restricted`.
6. **Authentication is present but authorization scope is incomplete.** `require_api_key` returns a `user_id`, but `/chat` accepts a caller-supplied `session_id` and uses it directly for session and Pinecone namespace selection. The user identity is not bound into that namespace in the inspected route.
7. **The API-key store is process-start environment state.** `APIKeyStore` reads a plaintext `API_KEYS` environment string into memory; rotation/revocation requires process reload and secrets handling is operationally sensitive.
8. **Session deletion is not vector deletion.** `DELETE /sessions/{session_id}` deletes the session-store history only. It does not delete or tombstone all Pinecone vectors, SQLite sidecar vectors, synaptic nodes/edges, or utility state for that session.
9. **Metadata ownership is not enforced.** `PineconeMemory.remember` merges caller metadata after reserved fields, permitting overwrites of `session_id`, `role`, `text`, deletion markers and other system-managed values.
10. **Budget claims exceed enforcement visible in the loop.** `max_run_usd` is configured but no provider-cost stop is enforced in the inspected agent loop. Token accounting and step limits are not equivalent to a hard monetary cap.
11. **Configuration/documentation drift exists.** README identifies v2.3 while `app/main.py` reports 2.4.0; neural health defaults to `data/utility.pt` while the utility model writes JSON to `data/utility_model.json`. Verify all advertised health paths and version metadata against runtime configuration.
12. **Neural recall wiring also needs input-shape verification.** `NeuralRecallPipeline.enhance` expects a query vector and Pinecone match-like objects with `id`, `score`, and `metadata`; the current memory caller supplies already-flattened dictionaries and no query vector. Fixing only the argument count would still leave a data-shape mismatch.

## HUSH integration boundary

Pinecone Neural may be used for a human-invoked research task that reads public documentation and reviewed repository snapshots. It must not be embedded into HUSH or gain direct access to local user sessions, project media, audio devices, credentials, or arbitrary filesystem paths.

- No Pinecone/OpenAI request, neural memory lookup, Python execution or model inference in the audio callback or realtime host.
- No automatic code checkout, branch mutation, commit, merge, shell command or deployment based on model output.
- No model-generated DAW command, plugin, MIDI, OSC or transport event is executed directly.
- No remote dependency or model download in CMake, install, startup or audio processing.
- All proposed source changes remain human-reviewed Git diffs on a branch, with CI and explicit merge approval.

## Professional DAW feature audit — scoring rules

The feature score remains a main-branch, end-to-end workflow score. A source primitive does not count as a complete DAW feature unless it is reachable by a discoverable command/protocol, validated transaction, undo/redo, versioned persistence/reopen, bounded realtime publication, UI observation, recovery, deterministic render and failure rollback.

The currently documented baseline is **30/100** against a Pro Tools Studio-class workflow baseline. This is a feature-coverage grade, not code quality and not a claim that HUSH is 30% of Pro Tools by source size.

## Transition gaps that matter most

| Transition | Present foundation | Missing link / required acceptance |
|---|---|---|
| Track assignment → project model | Tracks and buses/routes are persisted | Track has no persisted bus assignment ID; save/reopen cannot reconstruct track-to-bus topology |
| Project model → graph validation | BusGraph validates bus IDs, routes and cycles | Validate track destinations, required master/output, duplicate route semantics and project-wide topology |
| Validated graph → realtime graph | Realtime router and graph publisher primitives exist | One canonical prepared graph must feed Qt, ALSA and offline render; publish at block boundary and reclaim off callback |
| User command → project mutation | QAction registry/command palette exists on PR #99 | Stable command ID must invoke a validated domain transaction, not mutate widgets or runtime arrays directly |
| Mutation → undo/redo | Scalar gain/pan/mute/solo operations exist | Add atomic route/track/clip/playlist/insert/automation operations with exact before/after state |
| Mutation → durable state | JSON project persistence and recovery journal exist | Every new entity needs schema migration, reopen tests and crash recovery |
| Audio engine → UI | SPSC queues and meter foundations exist | UI-only observation, no GUI call/lock/allocation/logging from audio callback |
| MIDI input → musical event | CC-to-parameter learn map exists | MIDI device lifecycle, event model, scheduler, piano roll, CC/pitch/pressure/MPE lanes |
| Takes → comp | Recording ring and WAV writer/session primitives exist | Track arm, input mapping, take/playlist identity, comp ranges, crossfades and auto-color metadata |
| Plugin insert → sound | No production plugin host | Scanner, signed/verified metadata, process isolation, state, watchdog, latency reporting and PDC |
| Session → delivery | WAV export/render foundations exist | Batch export queue, range/marker/stem grouping, naming, cancellation, deterministic report |
| DAW → control surface | No complete control protocol | Stable command IDs, feedback state, banking, transport, fader and meter protocol contract |

## Pro Tools reference set and implementation priorities

The comparison uses Avid's current Pro Tools 2026.11 release documentation and reference guide, alongside the previously documented user-request research. The 2026.11 release specifically adds Extensions, local Stem Separator, MPE, alternate-take auto-coloring, and pinned-track focus switching. HUSH should treat these as separate workflow targets, not add superficial menu labels without end-to-end behavior.

Recommended implementation order:

1. **Canonical routing vertical slice:** persist track bus destination; validate it with the bus graph; use the same prepared graph for offline render and live engines; make route changes transactional/undoable; add migration, invalid-graph and deterministic output tests.
2. **Project-wide history:** create/delete/move/split/trim/route/insert/automation operations with stable IDs and rollback.
3. **Recording and playlists:** device/channel mapping, arm state, worker-thread file writes, take IDs, playlists, comp ranges and recovery.
4. **Arrangement and editing:** sample-accurate clip operations, edit modes, fades, nudge, markers and clip gain.
5. **MIDI and automation:** persisted event/automation models, piano roll, CC/velocity/pitch/pressure/MPE, deterministic scheduling and control surfaces.
6. **Plugin and delivery infrastructure:** isolated plugin host, state, watchdog, latency/PDC, batch stem export and interchange.
7. **Professional workflow commands:** expose each shipped feature through a stable command ID, menu, searchable palette, keymap and documented protocol.

## Validation status

- Pinecone Neural source was reviewed at `8260ee84fc6deb96aeebe46079d47948a30cd84d`.
- The Pinecone Neural service was **not launched** and no agent run, vector-index operation, code execution, PR creation by the agent, or Pinecone-generated patch is claimed.
- HUSH's existing branch contents were reviewed; no C++ implementation change is claimed in this review document.
- HUSH `main` remains unchanged. This review is documentation/review work only.
- Before any Pinecone Neural deployment is trusted, fix the tool-loop wiring, remove active `eval/exec`, enforce per-user session ownership, implement complete deletion, protect reserved metadata, correct neural call signatures/data shapes, and add adversarial and integration tests.
