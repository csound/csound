# Csound UGen API – Python Examples

These examples demonstrate the **UGen API** introduced in Csound 7, which
lets you create and run individual Csound opcodes as standalone unit
generators from a host language—no orchestra or score required for the
signal-processing parts.

The API is based on the design from:\
*"Extending Aura with Csound Opcodes"*\
Steven Yi, Victor Lazzarini, Roger Dannenberg, John ffitch — ICMC/SMC 2014

## Prerequisites

| Requirement | Notes |
|---|---|
| **Csound 7** | Built from the `feat/ugen-api` branch (or later, once merged) with the UGen API enabled. |
| **Python ≥ 3.10** | |
| **ctcsound.py** | Ships in the repo under `Python/ctcsound.py`. |
| **numpy** | Used by ctcsound for buffer access. |

## Quick start with `uv`

[uv](https://docs.astral.sh/uv/) manages a virtual environment and
installs dependencies automatically.  A `pyproject.toml` and `.env` file
are included so you can run the examples in one command:

```bash
cd examples/ugen
uv run --env-file .env python ugen_example.py
```

The `.env` file sets two variables relative to this directory:

| Variable | Default | Purpose |
|---|---|---|
| `PYTHONPATH` | `../../Python` | Makes `ctcsound` importable. |
| `DYLD_FRAMEWORK_PATH` | `../../build` | Locates `CsoundLib64.framework` (macOS). |

If your Csound build lives somewhere else, either edit `.env` or override
on the command line:

```bash
DYLD_FRAMEWORK_PATH=/path/to/build uv run --env-file .env python ugen_example.py
```

### Without `uv`

```bash
cd examples/ugen
pip install numpy                       # if not already installed
PYTHONPATH=../../Python \
DYLD_FRAMEWORK_PATH=../../build \
  python ugen_example.py
```

On Linux, replace `DYLD_FRAMEWORK_PATH` with `LD_LIBRARY_PATH` pointing
to the directory containing `libcsound64.so`.

## Examples

All eight examples live in **`ugen_example.py`** and run sequentially when
you execute the script.

### Example 1 – Single UGen (`oscils`)

Creates one `oscils` unit generator, sets amplitude / frequency / phase
using the convenience `set_value()` method, runs a single k-cycle, and
prints the first samples.  Shows the minimal lifecycle:
**create → set inputs → init → perform → read output → delete**.

### Example 2 – List opcodes

Queries the opcode database via `UgenFactory.list_opcodes()` and
`find_opcode()`.  Useful for discovering which opcodes are available at
runtime.

### Example 3 – Argument types

Inspects each input and output of a UGen via `UgenVar` handles to report
its rate type (`i`, `k`, `a`, `S`, `f`) and size in bytes.  Handy when
you need to know whether an opcode accepts k-rate modulation.

### Example 4 – UGen graph

Builds a graph of two `oscils` UGens (440 Hz + 660 Hz), initialises and
performs them together via `UgenGraph`, and reads both output buffers.
Uses convenience `set_value()` for init-time setup.

### Example 5 – Real-time vibrato (`oscili`, DAC output)

Plays a 440 Hz sine through the DAC with a 5 Hz vibrato (±40 Hz).  Uses
`oscili` which accepts **k-rate frequency**, so the frequency is updated
every k-cycle with no re-initialisation—phase stays continuous and the
pitch glide is smooth.  Demonstrates the two-level API: convenience
`set_value()` for init-time, cached `UgenVar` handle for per-k-cycle
updates.

### Example 6 – Real-time two-oscillator graph (`oscili`, DAC output)

Two `oscili` UGens in a graph play simultaneously.  The first holds a
steady 440 Hz; the second glides from 660 Hz down to 330 Hz over
3 seconds.  Demonstrates k-rate parameter update via cached `UgenVar`
inside a `UgenGraph`.

### Example 7 – UGen-to-UGen wiring (`oscili` LFO → carrier)

A k-rate LFO modulator feeds into a carrier oscillator's frequency
using cached `UgenVar` handles.  Because a base-frequency offset is
needed, the LFO output is read and the carrier input is written each
k-cycle manually.  Also shows how direct zero-copy wiring via
`set_input_var()` would work when no offset is required.

### Example 8 – String manipulation (`strcat`)

Demonstrates opcodes that process string (`S`-type) arguments.  Uses
convenience `set_string()` / `get_string()` for one-off operations, and
`UgenVar` string handles for direct access.  Also creates a standalone
string var via `UgenFactory.new_var()`.

## Key concepts

```
UgenFactory(cs)           – create UGens for a running Csound instance
  .new_ugen(name, ...)    – instantiate an opcode
  .new_graph()            – create an empty UgenGraph
  .new_var(arg_type)      – create a standalone UgenVar
  .list_opcodes()         – enumerate available opcodes

Ugen                      – one opcode instance
  .set_value(i, v)        – convenience: set scalar on input i
  .get_value(i)           – convenience: get scalar from output i
  .set_string(i, s)       – convenience: set string on input i
  .get_string(i)          – convenience: get string from output i
  .get_in_var(i)          – get UgenVar handle for input i
  .get_out_var(i)         – get UgenVar handle for output i
  .set_input_var(i, var)  – wire a UgenVar to input i (zero-copy)
  .init()                 – run the opcode's i-time code (call once)
  .perform()              – run one k-cycle of audio processing

UgenVar                   – typed variable handle (i/k/a/S/f)
  .set_value(v)           – set scalar value (i/k)
  .get_value()            – get scalar value (i/k)
  .set_string(s)          – set string value (S)
  .get_string()           – get string value (S)
  .data_ptr               – raw MYFLT* for audio/struct access
  .arg_type / .size       – type and size queries

UgenGraph                 – ordered collection of UGens
  .add(ugen)              – append a UGen to the graph
  .init() / .perform()    – init / perform all UGens in order
  .delete_all()           – free the graph and all contained UGens
```

### i-rate vs. k-rate opcodes

| Opcode | Freq input | Update strategy |
|---|---|---|
| `oscils` | **i-rate** | Must re-init to change frequency (resets phase). |
| `oscili` | **k-rate** | Update via `set_value()` or cached `UgenVar` each k-cycle—smooth, no re-init. |

For real-time modulation, prefer opcodes with k-rate inputs (like
`oscili`) so parameters can change continuously.

### Routing audio to the DAC

The UGen API generates audio buffers in memory.  To hear them, copy the
samples into Csound's input buffer (`cs.spin()`) and let a simple
pass-through instrument forward them to the DAC:

```python
orc = """
sr = 44100
ksmps = 64
0dbfs = 1
nchnls = 1
gifn ftgen 1, 0, 8192, 10, 1   ; sine table for oscili

instr 1
  asig in
  out asig
endin
"""
```

Each k-cycle: write UGen output → `spin`, call `cs.perform_ksmps()`.

## File listing

| File | Description |
|---|---|
| `ugen_example.py` | All eight examples. |
| `pyproject.toml` | uv/pip project (declares numpy dependency). |
| `.env` | Environment variables for `uv run --env-file`. |
| `README.md` | This file. |
