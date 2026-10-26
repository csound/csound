# Orchestra optimizer baseline

These fixtures and `../optimizer_bench.py` establish a reference before adding
optimizations. All fixtures are deterministic, mono, and require no sound files
or audio hardware. Three complete instruments are included: additive,
subtractive, and FM bell. Fixtures use instrument 1 and p4 as their frequency.

## Build and correctness

Use identical compiler flags for baseline/candidate builds. For strict arithmetic
on Clang/GCC, configure with:

```sh
cmake -S . -B build/optimizer -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBUILD_TESTS=ON -DBUILD_STATIC_LIBRARY=ON -DUSE_VCPKG=OFF \
  -DUSE_COMPILER_OPTIMIZATIONS=OFF -DUSE_DEFAULT_OPCODEDIR=OFF \
  -DCMAKE_C_FLAGS=-ffp-contract=off -DCMAKE_CXX_FLAGS=-ffp-contract=off \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build/optimizer --parallel
CSOUND7RC=/dev/null ctest --test-dir build/optimizer -R '^OrcOptimizer\.' --output-on-failure
CSOUND7RC=/dev/null cmake --build build/optimizer --target csdtests
```

Select the corresponding strict-FP options on other toolchains. On Windows,
point `CSOUND7RC` at an empty file instead of `/dev/null`. Avoid a local
`.csound7rc` in the working directory. Configure platform dependencies according
to `BUILD.md`; record additional flags. This project normally enables fast-math
through `USE_COMPILER_OPTIMIZATIONS`; `RelWithDebInfo` alone is not a strict build.

Per-engine options (experimental):

- `-+orc_opt_passes=0`: skip the optional legacy TREE optimizer.
- `-+orc_opt_passes=1`: existing default; only bit 0 is currently supported.
- `--verbose=0x200`: normalized input/output TREE and resolved opcode-template dumps
  through the `DEBUG_OPTIMIZER` bit in the existing debug mask.
- `--verbose=0x210`: combine optimizer and parser (`0x10`) diagnostics.
- Plain `--verbose` and `-v` retain their existing diagnostics and exclude optimizer
  dumps. An explicit mask containing `0x200` is required, including when combined
  with other categories. `DEBUG_FULL` retains its existing value; explicitly passing
  `--verbose=0x7fffffff` includes optimizer dumps too.

These do not disable semantic lowering, type checks, UDO rate classification,
getter phase specialization, or phase diagnostics. They reset with the engine.
Configure them before compilation; do not mutate controls concurrently with a
compile. Invalid masks are rejected by the configuration-variable API; the
existing CLI option parser reports configuration errors as warnings and may still
return success. Tests check the underlying API and actual stored controls.

Dumps renumber generated temporaries/labels per scope and expose resolved opcode
name/signature, callback phases, operation counts, and template storage counts.
Operands carry their resolved metadata types, such as `control:k`, `values:k[]`,
and `1:c`, alongside the registration signature (which may contain wildcards).
`p` denotes a p-field and `?` denotes unavailable type metadata; types are read
without rerunning semantic analysis.
TREE dumps preserve structured member references (including nested read/write
bindings such as `thing.field.value`) instead of leaving an empty operand slot.
TREE phases are provisional. `phase=resolved` follows UDO rate classification.
Callback presence is not a proof of purity or actual execution count. Storage
metrics describe variable-pool/template payloads, not allocator overhead, opcode
auxiliary storage, dynamic arrays, nested UDO instance totals, or peak RSS. They
must not be described as measured total instance allocation. Dumps are diagnostic,
not a public interchange format, and are excluded from timed runs.

## Paired throughput matrix

On macOS this build produces a framework; on other systems use the shared-library
path from that build. Example:

```sh
CSOUND7RC=/dev/null python3 tests/benchmarks/optimizer_bench.py \
  --library build/optimizer/CsoundLib64.framework/CsoundLib64 \
  --build-dir build/optimizer --output /tmp/optimizer-baseline.json
```

Default matrix: all nine fixtures, `ksmps` 1/16/64/256, voices 1/32, 48 kHz,
0.25 rendered seconds, three warmups, and 21 paired trials. Alternate off/on order
on every trial. Use `--cases audio_chain repeated`, `--ksmps 64`, or `--voices 1`
for focused investigations. Repeat the invocation in a separate process when
confirming a result. Do not run builds/tests or other benchmarks concurrently.

Every cell first verifies bit-identical full captured audio (including the initial
activation block) and pre-destruction diagnostics in fresh off/on engines; dumps are retained
separately and may legitimately differ after future passes. Timed trials do not
capture audio or dump opcodes. Dump collection uses separate compile-only engines;
the `ORC-OPT` blocks exclude unrelated verbose messages and their raw TREE details.
A final `complete: true` marker distinguishes finished files from checkpoints.
Output contains raw paired samples, median times,
a deterministic paired bootstrap interval for the on/off ratio, source, seeds,
library hash, revision/dirty status, platform, CMake cache, and actual arithmetic
compile commands. Keep source diffs with a report for dirty revisions.

Metrics:

- `compile_ms`: orchestra compile; engine creation/options are excluded.
- `start_ms`: engine start and instrument 0 initialization.
- `activation_block_ms`: first block, including event consumption, note activation,
  initialization, and one performance block. This is NOT pure note-init time.
- `render_ms`: subsequent steady blocks, excluding setup, capture, and file I/O.
- `live_compile_ms` (with `--live-compile`): synchronous compile of a small new instrument after rendering,
  while notes are still active; not an asynchronous publication benchmark.
- `samples_per_second`: mono rendered sample frames per second, not per-voice samples.

Python/ctypes call and loop overhead are included in render time, especially
noticeable at `ksmps=1`. Use this consistently for comparisons; confirm small
native-kernel gains with a native timing loop before making performance claims.
The first activation block is separate; repeated cold engine trials are not a
steady note-allocation benchmark. Record these limitations when interpreting data.

## Live-compilation experiment

Use `--live-compile --cases audio_chain` in a separate invocation. Live compilation
is excluded from the default render matrix. The untouched reference crashes during
engine destruction after live compilation with an active local-ksmps UDO; the runner
rejects that known failing combination. The other render fixtures remain available.
This failure was reproduced on reference `215d8c0c56b6fefd4be641dc001c4262e4eda45f`.

## Block-time experiment

Run separately from throughput measurements. At least 10,000 blocks per trial
are enforced; for example:

```sh
CSOUND7RC=/dev/null python3 tests/benchmarks/optimizer_bench.py \
  --library build/optimizer/CsoundLib64.framework/CsoundLib64 \
  --build-dir build/optimizer --output /tmp/optimizer-block-times.json \
  --cases audio_chain --ksmps 64 --voices 32 --seconds 14 \
  --block-times --repeats 3 --warmups 1
```

Records every block duration, p50/p95/p99/max and deadline exceedances relative to
`ksmps/sr`. These are unscheduled host-driven block calls with timer/ctypes overhead,
NOT hardware callback measurements or proof of real-time safety. An empty-instrument control estimates the combined engine/ctypes/timer floor;
retain raw timings rather than subtracting that estimate. Actual hardware callback
tails remain required before a real-time claim.

## Compile scaling and untouched reference

```sh
CSOUND7RC=/dev/null python3 tests/benchmarks/compile_expression_args.py \
  --library build/optimizer/CsoundLib64.framework/CsoundLib64 \
  --phase compile --passes 0 > /tmp/optimizer-compile-off.json
CSOUND7RC=/dev/null python3 tests/benchmarks/compile_expression_args.py \
  --library build/optimizer/CsoundLib64.framework/CsoundLib64 \
  --phase parse --passes 1 > /tmp/optimizer-parse-on.json
```

Run both phases and masks, then repeat against an untouched baseline build with
identical flags, omitting `--passes` (older libraries lack the control). The default
argument counts are 8/64/256/1024. Compare raw trials and absolute latency; do not
infer scalability from a single small instrument or treat noisy CI timings as gates.
