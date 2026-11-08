# memplay

`memplay` plays an entire sound file from the engine's shared memory cache. It
supports the same interpolation, pitch changes, reverse playback, bounded loops,
and loop crossfades as `diskin2`.

```csound
aLeft, aRight memplay "stereo.wav", 1

aChannels[] memplay "multichannel.wav", 1

; Crossfade over 128 output frames while looping from 0.2 to 0.8 seconds.
aLoop[] memplay "stereo.wav", 1, 0.2, 128, 0, 4, 0, 0, 0, 0.8
```

The argument order matches `diskin2`, so an existing call can change just the
opcode name:

```text
memplay Sfile [, kpitch, iskiptime, iwrap, iformat, iwsize,
              ibufsize, iskipinit, iforceSync, iend]
```

Numeric file arguments also work through `strset` or `soundin.N`, as in `diskin2`.
Scalar output supports up to 40 channels; array output uses the file's channel
count. Extra scalar outputs are silent.

- `kpitch` defaults to 1. Negative values play backwards; zero holds the position.
- `iskiptime` defaults to 0 seconds.
- `iwrap` is 0 for one-shot playback, 1 for hard looping, or greater than 1 for a
  crossfade of that many frames. Crossfades shorten the effective loop.
- `iformat` and `iwsize` follow `diskin2`; the default interpolation is cubic.
- `ibufsize` controls the small interpolation buffers. Refills copy memory only.
- `iskipinit` preserves the current reader on reinit when nonzero.
- `iforceSync` is accepted for compatibility and ignored: `memplay` always uses
  synchronous memory access, including with `--realtime`.
- An explicit `iend` bounds playback in seconds. With looping enabled, the loop
  runs from `iskiptime` to `iend`, following `diskin2`'s boundary rules.

The first initialization loads and decodes the whole file through `LoadSoundFile`
in `Engine/memfiles.c`, the sound-file form of the engine's memory-file mechanism.
It closes the disk file before playback starts. Later instances using the same
file name share its decoded samples. Playback does no file I/O, allocation, or
reader-thread work. Initializing an uncached file can still block while loading it.

The engine retains cached files until reset. Changing the file on disk does not
reload an existing cache entry. Decoded storage takes approximately
`frames * channels * sizeof(cs_float)` bytes, plus each reader's small working buffers.
There is no automatic fallback to streaming; use `diskin2` for files that should
not stay in memory. The reader supports at most `INT32_MAX` frames.

`tests/commandline/test_memplay_short_loop.csd` is a self-contained example that
creates its own source and checks short loops, wide interpolation windows, and
fast playback.
