# HUSH 0.2 — Recovery journal contract
## API
`hush::RecoveryJournal` appends full project snapshots to a local journal and restores the newest complete, parseable snapshot. It is explicit-call only; an application lifecycle must call it from a non-audio thread.

## Record framing
Each record is `HUSHJ1 <decimal byte count>\n<payload JSON>\n`. Length framing lets recovery detect a truncated final record without mistaking it for a complete snapshot. The implementation caps each record at 64 MiB and stops at the first malformed or incomplete record, retaining the newest prior valid snapshot.

After writing, the implementation flushes the stream and requests `fsync` on POSIX or `_commit` on Windows. This is not a guarantee against every filesystem, storage-controller, or power-loss failure; platform-specific crash testing remains required. It does not yet rotate/compact journals or schedule autosaves.

## Safety boundary
Do not call journal functions from an audio callback: they allocate, serialize, and perform file I/O. A failed stable-storage sync is reported as an error even if the record bytes were written.
