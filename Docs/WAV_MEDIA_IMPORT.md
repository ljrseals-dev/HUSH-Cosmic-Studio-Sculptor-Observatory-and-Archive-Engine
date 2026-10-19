# Offline WAV media import foundation

The read_wav API decodes little-endian RIFF/WAVE files to normalized interleaved float samples on an import/control thread.

## Supported format

- PCM signed 16-bit integer.
- IEEE 754 32-bit float (non-finite samples are converted to silence).
- 1–8 channels; the original channel count and sample rate are preserved.
- Ancillary RIFF chunks are skipped within the declared RIFF boundary.
- Default audio-data limit is 512 MiB; callers may configure this with WavReadOptions.

The reader rejects truncated headers/chunks/data, invalid block alignment, unsupported sample encodings, invalid channel layouts, and data beyond the configured limit. It does not resample, downmix, or add a clip to a project.

## Threading and scope

Reading allocates memory and performs filesystem I/O. It must never run in the realtime callback. The returned buffer is an imported media object only; schema-v1 Project still has no media-source IDs or sample-positioned clips. Timeline placement and arrangement rendering remain subsequent work.

No runtime network access or external decoder dependency is introduced.
