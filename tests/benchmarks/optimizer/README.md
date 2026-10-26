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

- `--optimize=none` disables the optional passes.
- `--optimize=legacy` or `--optimize-legacy` enables the existing TREE pass.
  This is the default.
- `--optimize=constant-folding` or `--optimize-constant-folding` enables constant folding.
- `--optimize=copy-removal` or `--optimize-copy-removal` enables scalar copy removal.
- `--optimize=legacy,copy-removal` enables both named passes.
- `--optimize=legacy,constant-folding` enables both named passes.
- `--optimize-common-subexpression-elimination` enables local scalar CSE.
- `--optimize=all` enables every pass implemented in this build, including
  experimental passes. Constant folding has a reported steady-render slowdown
  on macOS. Measure your workload before enabling it.

Named lists and individual flags add to the current selection. Options take effect
in command-line order. Use `--optimize=none` first to select a pass without the
legacy optimizer. `all` and `none` must appear alone after `--optimize=`.
Unknown names, missing names and empty list entries produce an option error. A
bad name anywhere in a list leaves the selection unchanged. Pass names select
which transformations run, not their execution order.

The benchmark runners keep numeric labels in their output to compare with earlier
measurements. They translate those labels to the named options before compilation.
The bits are 1 for legacy, 2 for constant folding, 4 for copy removal and 8 for
CSE. Selecting all four gives 15. The CSE comparison still uses 1 and 9 to keep
constant folding and copy removal off.

The earlier `orc_opt_passes` configuration variable was an unreleased benchmark
control. These options replace it, so callers of that control must switch to
the names above. Public function signatures and structure layouts stay unchanged.

Diagnostic options:

- `--verbose=0x200`: normalized input/output TREE and resolved opcode-template dumps
  through the `DEBUG_OPTIMIZER` bit in the existing debug mask.
- `--verbose=0x210`: combine optimizer and parser (`0x10`) diagnostics.
- Plain `--verbose` and `-v` retain their existing diagnostics and exclude optimizer
  dumps. An explicit mask containing `0x200` is required, including when combined
  with other categories. `DEBUG_FULL` retains its existing value; explicitly passing
  `--verbose=0x7fffffff` includes optimizer dumps too.

The optimizer controls do not disable semantic lowering, type checks, UDO rate
classification, getter phase specialization, or phase diagnostics. They reset
with the engine.
Configure them before compilation; do not mutate controls concurrently with a
compile.

Literal folding handles resolved built-in scalar addition, subtraction and
multiplication. It substitutes a generated i-rate temporary only when one
known, init-only consumer reads it in the same straight-line block. Other
opcodes and control-flow boundaries stop propagation. Division, non-finite and
subnormal arithmetic, overloads and performance-time consumers remain unchanged.
Each operation uses sample precision and keeps its original grouping. Unused
variable-pool slots remain allocated. This pass is experimental and stays off by
default.

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

Use `--live-compile --cases audio_chain udo` in a separate invocation. Live
compilation is excluded from the default render matrix, but all fixtures are
supported in this experiment. Active local-ksmps UDOs survive table relocation and
engine cleanup; `UdoLiveCompileTests` covers this along with inactive-frame reuse.
Reference `215d8c0c56b6fefd4be641dc001c4262e4eda45f` has a table-growth/relocation
bug that can crash this sequence, so use a build containing the cleanup fix.

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

## Scalar copy removal

`--optimize=copy-removal` adds copy removal to the default `legacy` pass.
Use `--optimize=none --optimize-copy-removal` to run copy removal alone.

The pass lets built-in i-rate addition, subtraction and multiplication write a
local scalar destination directly when the next operation copies their generated
result there. The result must have exactly one use. This runs after overload
resolution and leaves plugins, UDOs, globals, other rates and structured values
alone. It does not change arithmetic order or remove temporary variable slots.
Default compilation remains `--optimize=legacy`.

`--optimize=all` includes this experimental pass. The recorded macOS runs
showed a 26–32% slowdown in the 32-addition workload, although that machine
was also running other tasks. Those results need a repeat on an idle machine.
The large workload improved on both hosts, and the Linux 32-addition workload
also improved. Keep the pass opt-in and measure your own workload before
enabling it.

The native `optimizer_timeline` benchmark measures repeated note starts and audio
processing in consecutive 1.024-second score windows. Each note lasts 16 blocks
at 48 kHz with ksmps 64. Three warmup windows precede measurement. The fixture
generates a chain of local i-rate additions whose final value sets a sine tone's
frequency. This
is a synthetic initialization workload, not a prediction for all instruments.

```sh
ninja -C build/optimizer optimizer_timeline
CSOUND7RC=/dev/null python3 tests/benchmarks/optimizer_timeline.py \
  --binary build/optimizer/tests/benchmarks/optimizer_timeline \
  --library build/optimizer/CsoundLib64.framework/CsoundLib64 \
  --build-dir build/optimizer --copies 512 --voices 128 --windows 12 \
  --output /tmp/scalar-copies.json
```

The runner checks complete audio and messages in separate untimed runs, then
alternates masks 1 and 5 in fresh processes for each pair. The native loop
reports the loaded library, checks sample precision and reads back the pass mask.
The runner rejects a library that differs from `--library`. The native loop
measures process CPU and wall time, including event submission, note activation
and audio processing. Compilation is measured separately. A copy count of zero
is a control for the harness and scheduling cost. Keep generated reports and
charts outside the source tree. Record the compiler, CPU and build flags with
results. Run on an idle machine and pin the runner to one CPU on Linux if useful.

## Literal folding

The [recorded macOS and Linux comparison](../results/literal-folding/README.md)
includes initialization gains, compile costs and a steady-rendering regression.

Run the focused comparison on macOS or Linux with the same strict build above.

```sh
CSOUND7RC=/dev/null python3 tests/benchmarks/literal_folding.py \
  --library build/optimizer/CsoundLib64.framework/CsoundLib64 \
  --build-dir build/optimizer --output /tmp/literal-folding.json
```

On Linux use the built `libcsound64.so` path. A CPU affinity tool such as
`taskset -c 14` can pin the runner and its children to one available core.
Run this without another build or benchmark on the same host.

The runner compares mask 1 against mask 3 in fresh processes, alternating order,
with three warmup pairs and 21 measured pairs. It uses 128 simultaneous notes at
48 kHz and ksmps 64. The cases are a small musical instrument, a dynamic-input
control, and generated instruments with 64 or 512 literal expressions. Every
generated value contributes to the output. Use `--voices 1` to check the small
case separately.

The activation measurement includes fresh note allocation, initialization, and
one audio block. It does not isolate arithmetic dispatch or instance reuse.
Compilation, engine start, activation, and one second of steady rendering each
have separate wall and process CPU times. The steady rendering loop includes
Python/ctypes overhead. Audio and ordered diagnostics must match in separate
untimed capture runs before timing starts. Dumps and raw paired measurements
remain in the JSON report with build flags and the library hash.

Memory readings are process peak RSS at each stage, including Python, the loaded
library, compiler storage, and allocator overhead. They are cumulative high-water
marks, not current memory or per-note allocation. Fresh processes keep one
candidate's allocations out of the other's measurement. Do not describe retained
variable-pool slots as reclaimed memory, or infer a steady DSP speedup from a
lower activation time.

## Scalar common-subexpression elimination

CSE reuses resolved built-in scalar addition, subtraction and multiplication
inside instruments. It compares the exact implementation, ordered operands and
execution phase. Writes invalidate saved inputs or results. Calls, labels,
jumps, compound assignments such as `+=`, and phase changes end reuse. It leaves
globals, UDO bodies, audio, arrays, structs, division and overloaded operations
alone.

The first version only removes a generated result with one definition and all
its reads in the next known arithmetic or scalar-copy operation in the same
phase. Named assignments retain their source locations and bindings. This
restriction prevents a temporary alias from crossing an unknown call or branch.
It can still reuse nested expressions. A 64-entry table bounds lookup cost.
Unused temporary slots remain in the variable pool. Cascading replacements can
also leave computations whose results are no longer read. Removing those
computations needs a separate dead-code pass. The default stays unchanged.

### Native Linux comparison

```sh
ninja -C build/optimizer scalar_cse_bench
CSOUND7RC=/dev/null taskset -c 14 python3 tests/benchmarks/scalar_cse_bench.py \
  --binary build/optimizer/tests/benchmarks/scalar_cse_bench \
  --library build/optimizer/libcsound64.so --build-dir build/optimizer \
  --expressions 512 --output /tmp/cse-repeated-512.json
```

Each generated expression is `(input + offset) * 0.75 - 0.25`. Its result goes
into `sum = sum + value`, which controls an oscillator. Explicit addition keeps
this within the initial allowlist, while `sum += value` would end reuse. Repeated cases use one offset. `--unique`
uses a distinct offset for each expression as a negative control. Also run
`--expressions 0`, `--expressions 32`, and `--rate i` to separate steady control
work from note initialization.

The runner labels the legacy-only and legacy-plus-CSE selections as 1 and 9
in its raw data. It sets those selections through the named options and compares them in 21 alternating pairs of fresh processes.
It checks the loaded library path and sample size. Separate capture runs compare
all samples and ordered messages before timing. Each process schedules 32 voices,
measures the activation block, warms up for three windows, then records 12
windows of 128 blocks at 48 kHz and ksmps 64. CPU and wall time come from a native
loop. The chart's time axis is score time after warmup, not real-time playback.
Compilation and the first block are reported separately. These synthetic cases
measure callback savings, not whole-composition or hardware audio latency.

Pin the process to a quiet CPU and avoid concurrent builds or benchmarks.
Record the host, compiler flags and precision. Output includes source, revision,
local diff, binary/library hashes, CMake configuration and raw paired timings.
Keep results and charts outside the repository.
