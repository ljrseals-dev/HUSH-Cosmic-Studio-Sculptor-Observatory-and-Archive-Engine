# HUSH Prompt 4 (0.4a) acceptance ledger

The authoritative prompt is Docs/Prompt, section Prompt 4 — HUSH 0.4.

| Requirement | Status | Evidence required |
|---|---|---|
| Preallocated ring and off-thread writer | Foundation implemented | Build and contract tests |
| Finalized streaming WAV | Foundation implemented | Decode and exact sample/frame comparison |
| Multitrack capture | Not complete | Independent synchronized track capture |
| Take UUIDs, playlist, active take | Not implemented | Project schema round-trip and stable-ID tests |
| Manual/auto/loop punch, pre/post-roll | Not implemented | Sample-frame boundary tests within ±1 sample |
| Comp overlay and region selection | Not implemented | Bit-identical manually assembled reference |
| Crossfades | Not implemented | Defined curve/boundary and render tests |
| Input latency and compensation | Not implemented | Named-device loopback report and persisted offset |
| 8 tracks × 10 minutes, zero drops | Not run | Named hardware, full frame counts, zero drops, playback comparison |
| Prior gates | Not re-run | Full CI matrix and target-machine validation |

Do not release Prompt 4 as complete until every binding gate passes. This ledger intentionally distinguishes implemented foundation from outstanding product requirements.