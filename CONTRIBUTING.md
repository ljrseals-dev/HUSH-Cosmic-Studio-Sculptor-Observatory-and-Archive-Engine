# Contributing to HUSH Cosmic Studio

## Commit messages

Use concise, imperative, scoped messages. Examples:

- `repo: add ignore rules for generated artifacts`
- `docs: document recovery-journal limits`
- `processor: preallocate bus scratch`
- `tests: cover truncated journal tails`
- `ci: reject committed build artifacts`

Do not use vague messages such as `Rename X to Y` for new work. Historical messages remain unchanged.

## Branch naming

Use a short type/scope prefix, for example:

- `repo/<topic>`
- `docs/<topic>`
- `processor/<topic>`
- `tests/<topic>`
- `ci/<topic>`

Open a pull request against `main`; do not force-push or rewrite published history without explicit maintainer approval.

## Offline-build rule

The project build must not download or bootstrap dependencies. Do not add CMake `FetchContent`, curl/wget, Git clone, package-manager bootstrap scripts, runtime network access, telemetry, crash uploads, update checks, or remote configuration. Optional dependencies (Qt 6, ALSA development headers) must already be installed before CMake configuration. CI may install system packages on the runner before building.

## Tests and evidence

Run the documented default build and `ctest`; also run affected optional configurations. Add tests for new contracts. Hardware latency, xrun, performance, soak, and durability claims require reproducible reports from the named target machine; CI compilation alone is not evidence for those claims.

## Binary assets

Do not commit ZIP archives, build products, generated libraries, or test output. Publish binary archives as GitHub Release assets. If an archive is required for a source import, inspect it, extract required source/docs into the canonical tree, and record provenance before removing the archive.
