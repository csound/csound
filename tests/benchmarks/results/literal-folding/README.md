# Scalar literal folding measurements

Measured on 26 October 2026 at `6a66963f125a6991fedb4b22f0309556b3201a92`.
Later changes add boundary cases and this report, without changing the optimizer
or measured library. This is an opt-in compiler pass, not a default change.

Literal-heavy instruments spend less CPU time activating notes, but compilation
can cost more. A small musical instrument shows no clear activation gain.
A single note does not repay the compile cost in the largest generated case.

## Setup

Both builds use double samples, Release `-O3`,
`USE_COMPILER_OPTIMIZATIONS=OFF`, and `-ffp-contract=off`.
They use the same source revision. Libraries differ by platform and optional
dependencies, so compare off and on within each host, not raw times across hosts.

- macOS 27.0.1 on Apple M5 Max, Apple Clang 21.0.0, no CPU pinning.
- Estia Linux on Ryzen 9 7950X, GCC 14.2.0, pinned to CPU 14. The host also
  runs other services. No Csound build or other benchmark ran at the same time.

The runner compares pass mask 1 with mask 3 in fresh processes, alternating order.
Each case has three warmup pairs and 21 measured pairs. The main runs start 128
notes together at 48 kHz with ksmps 64. Separate runs use one note. Each generated
value contributes to the output. The musical case has an envelope and oscillator.
The dynamic control uses p-fields, so its arithmetic cannot fold.

Activation includes fresh note allocation, initialization and one audio block.
It does not measure reuse of old instances. Audio and ordered messages match
between masks in separate capture runs. Dumps and capture runs are outside the
timed intervals.

The following times are process CPU milliseconds. Percentages are medians of
paired ratios, not ratios of the two displayed medians. These results describe
these runs only, not machine-to-machine variation.

## Compilation and activation

| Host | Case | Compile off → on (ms) | Paired change | Activation off → on (ms) | Paired change |
| --- | --- | ---: | ---: | ---: | ---: |
| M5 Max | musical | 0.841 → 0.872 | -0.5% | 0.490 → 0.487 | -2.1% |
| M5 Max | dynamic64 | 1.426 → 1.480 | +3.2% | 0.879 → 0.869 | -0.9% |
| M5 Max | literal64 | 1.420 → 1.413 | -0.9% | 0.885 → 0.592 | -33.0% |
| M5 Max | literal512 | 7.923 → 8.132 | +3.0% | 5.718 → 3.664 | -34.9% |
| 7950X | musical | 1.451 → 1.444 | -0.5% | 1.793 → 1.777 | -0.9% |
| 7950X | dynamic64 | 2.411 → 2.459 | +1.9% | 2.741 → 2.737 | -0.3% |
| 7950X | literal64 | 2.555 → 2.491 | -2.7% | 2.813 → 2.012 | -28.1% |
| 7950X | literal512 | 12.472 → 15.304 | +22.0% | 16.659 → 9.962 | -40.2% |

For `literal512` with one note, macOS activation falls from 0.063 to 0.047 ms,
while compilation rises from 8.157 to 8.387 ms. Linux activation falls from
0.163 to 0.096 ms, while compilation rises from 12.546 to 15.359 ms.
The setup cost matters for short scores with few notes.

## Steady rendering

The pass removes init-only arithmetic. It does not optimize the audio loop.
This measurement renders one second of audio after activation and includes the
Python/ctypes loop overhead.

| Host | Case | Off → on (CPU ms per audio second) | Paired change |
| --- | --- | ---: | ---: |
| M5 Max | musical | 11.888 → 11.617 | -3.6% |
| M5 Max | dynamic64 | 3.488 → 3.467 | +0.2% |
| M5 Max | literal64 | 3.481 → 3.415 | -2.6% |
| M5 Max | literal512 | 3.645 → 4.110 | +11.6% |
| 7950X | musical | 12.503 → 12.137 | -3.0% |
| 7950X | dynamic64 | 3.666 → 3.698 | -0.1% |
| 7950X | literal64 | 3.602 → 3.581 | -0.1% |
| 7950X | literal512 | 3.590 → 3.612 | +0.6% |

The macOS `literal512` slowdown repeated in a second run, from 3.714 to 4.171 ms
per audio second, with a paired increase of 12.6%. Its cause is not isolated.
The change to instance allocation and layout is a possible cause, not a proven
explanation. Do not dismiss this as noise or claim a general DSP speedup.

## Memory

These are cumulative process peak RSS readings from fresh processes. They include
Python, shared libraries, compiler storage and allocator overhead. They do not
measure current live allocations or per-note variable storage. The pass leaves
variable-pool slots in place even when it removes arithmetic opcode instances.
The runner also records peaks after compilation, engine start and rendering.

## Reproduce

Use the strict build instructions in [the optimizer guide](../../optimizer/README.md).
Then run from the repository root, replacing the library path on Linux with
`build/optimizer/libcsound64.so`.

```sh
CSOUND7RC=/dev/null python3 tests/benchmarks/literal_folding.py \
  --library build/optimizer/CsoundLib64.framework/CsoundLib64 \
  --build-dir build/optimizer --output /tmp/literal-folding.json \
  --repeats 21 --warmups 3 --voices 128
```

On Linux prefix the command with `taskset -c 14` after setting the environment
variable, or choose another available CPU. Repeat with `--voices 1` for the
single-note comparison. Add `--case literal512` for the repeated large case.
Each worker has a five-minute timeout. Use `--timeout SECONDS` for slower
builds. A timeout stops the run and leaves its report marked incomplete.

The runner saves sources, dumps, audio hashes, messages, raw pairs, build flags
and library hashes in its JSON report. Keep generated reports and charts outside
the repository.

To draw charts, use complete reports from macOS and Linux with 128 notes,
21 measured pairs and all four cases in the default order. Focused `--case`
runs and other counts cannot use this fixed chart layout. The script rejects
them before writing charts. Install Matplotlib in a temporary environment
and run from this folder.

```sh
python3 plot.py /tmp/literal-folding-charts \
  /tmp/literal-folding-macos.json /tmp/literal-folding-linux.json
```

The script writes HTML with inline SVG. The light palette uses system fonts so
the charts also work offline.
