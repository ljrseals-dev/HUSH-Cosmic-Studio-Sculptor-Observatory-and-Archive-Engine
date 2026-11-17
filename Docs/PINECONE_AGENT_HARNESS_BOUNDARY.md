# Pinecone Agent Harness — HUSH Integration Boundary

## Decision

Use `ljrseals-dev/pinecone` as an optional, out-of-process research and documentation assistant only. Do not link it into the HUSH C++ engine, Qt process, ALSA host, project loader, or audio callback.

Repository reviewed: https://github.com/ljrseals-dev/pinecone  
Reviewed ref: `main`  
Reviewed tree: `3b9ba3091b95ecbd8f7d57efbb844aa9227e2e8d`

This is a Python/FastAPI agent harness with OpenAI model calls, Pinecone vector memory, and a structured `tool_call` / `final_answer` loop. It is not a C++ DAW subsystem, a GitHub execution agent, or a sandboxed code runner.

## Source-level security findings

The current source must not be treated as a safe command-execution boundary:

1. `app/harness.py::execute_restricted` compiles and evaluates model-supplied Python with `eval` and falls back to `exec`. A restricted builtins dictionary is not a Python sandbox; object introspection and reachable objects can defeat this boundary.
2. `tool_timeout_s` is configured but is not enforced around `eval` / `exec`. The output byte cap is not a CPU, memory, syscall, or side-effect limit.
3. `app/main.py` exposes chat/session/memory endpoints without an authentication dependency. CORS defaults to `*`; the API binds to `0.0.0.0` by default.
4. The harness creates outbound OpenAI and Pinecone clients and can create a remote Pinecone index. It therefore does not preserve HUSH's offline/no-network execution guarantee.
5. The `metadata` argument in `PineconeMemory.remember` is expanded after reserved metadata fields, so callers can override fields such as `session_id`, `role`, `text`, or deletion state unless validated.
6. `DELETE /sessions/{session_id}` removes only the in-process session object; it does not erase that namespace's vector records.
7. `max_run_usd` is configured but no cost enforcement is visible in the agent loop. The current token counter is not a hard external billing cap.

These are source-review findings, not claims that the service has been exploited.

## Permitted use around HUSH

- Run only as a separately operated research service with explicit operator invocation.
- Use it to retrieve and summarize public DAW documentation, issue discussions, protocol references, and already-approved repository notes.
- Treat every retrieved memory and model response as untrusted input.
- Require a human to inspect and approve every proposed patch.
- Apply approved changes through a normal Git branch and pull request.
- Keep all build, test, packaging, and deployment commands in the repository's normal CI or a separately controlled local environment.

## Prohibited integration

- No Pinecone/OpenAI client, HTTP request, vector lookup, agent loop, or model inference in HUSH's audio callback or realtime host.
- No model-generated shell, Python, C++, Qt slot, plugin, MIDI, OSC, or transport command is executed automatically.
- No agent access to HUSH project files, user media, credentials, audio devices, or arbitrary local filesystem paths.
- No remote dependency download or service startup from HUSH's CMake/build/install path.
- No AI result is allowed to mutate a project without validation, a domain transaction, undo/redo, persistence, and explicit user confirmation.

## Required hardening before any shared deployment

1. Replace `eval` / `exec` with a fixed, typed tool registry. Each tool must validate a schema and expose only a narrow operation.
2. If untrusted code execution is truly required, move it to a separately administered OS/container/VM sandbox with enforced CPU, memory, filesystem, network, process, and wall-clock limits. Do not rely on Python builtins filtering.
3. Add authentication, authorization, rate limits, request-size limits, and restrictive explicit CORS origins.
4. Validate namespace/session identifiers and reserve metadata keys; reject caller overrides of system-owned fields.
5. Implement actual deletion/purge semantics and audit retention behavior for vector memory.
6. Enforce cost and token budgets against provider usage, with bounded retries and a hard stop.
7. Add adversarial tests for Python escape attempts, prompt injection, namespace isolation, metadata spoofing, denial of service, and deletion guarantees.
8. Keep secrets out of prompts, logs, vector metadata, and returned error details.

## HUSH feature integration rule

Pinecone is a research aid, not a DAW feature. HUSH's professional-workflow implementation remains native C++20 and local: stable command IDs, validated project transactions, complete undo/redo, versioned persistence/recovery, bounded realtime publication, UI-thread observation, and deterministic offline rendering. The agent may suggest a change; HUSH itself must enforce the full domain contract.

## Validation status

This document is a static source review of the referenced Pinecone repository revision. The harness was not launched: no Pinecone/OpenAI credentials or isolated runtime were provided in this work session. No Pinecone tool call, remote index creation, agent execution, or HUSH integration is claimed.
