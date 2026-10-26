# DSP benchmarks

## poscil

Build a Release configuration, then request the optional target. It is excluded
from the default build and does not run as a CI test.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build poscil_benchmark
build/tests/benchmarks/poscil_benchmark 64 64 65536 440 60
```

Arguments are `ksmps`, voice count, table length, base frequency and duration
in seconds. The sample rate is 48 kHz. Each voice runs one `poscil` with k-rate
amplitude and frequency, then mixes its output. Voice frequencies differ by
0.25 Hz. The benchmark uses the actual built library and its sample type.

Only the `csoundPerformKsmps` loop is timed, after a one-second warmup rounded
up to a whole block. Setup, compilation and teardown are excluded. Audio device
and file output are disabled. This measures engine dispatch and mixing as well
as the opcode, rather than an isolated interpolation kernel. `elapsed_seconds`
uses a monotonic clock. Run on an otherwise idle machine and increase duration
if timing noise obscures the difference.

Build the baseline and candidate with identical flags in separate worktrees.
Alternate their runs, repeat each case at least seven times, and compare medians.
Record the compiler, CPU, precision and flags. Useful cases include block sizes
1, 16, 32, 64 and 256, one and many voices, table lengths 1000 and 65536, and
positive and negative frequencies. Large frequencies also exercise multiple
table wraps per sample. Do not use emulated x86 timings to justify an AVX2 path.

Timing alone does not establish correctness. Run
`test_poscil_block_phase.csd` and `test_poscil_sample_bounds.csd` in the command-line
suite. Compare rendered samples and long runs against the scalar version in
each precision mode before adopting a faster path. In particular, replacing
repeated phase additions with `phase + lane * increment` changes rounding and
can accumulate phase drift. Preserve the guard-point read, negative-frequency
wrapping, non-power-of-two tables and inactive samples at note boundaries.
