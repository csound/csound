# Numeric types in Csound 7

Use `cs_float` for samples, opcode arguments, and values passed to the Csound
API. Use `cs_double` for calculations that normally need double precision.
Include `csound.h` in hosts or `csdl.h` in plugins; both expose these types.
Standalone code can include `csound_types.h`.

`MYFLT` remains a typedef alias for `cs_float` in Csound 7. Existing C and C++
code still compiles, but MSVC, GCC, and Clang warn when it uses `MYFLT`.
Replace the type name with `cs_float` to remove the warning. Projects that
treat warnings as errors must migrate or allow deprecated declarations.
`MYFLT` is now a typedef, so do not test it with `#ifdef` or redefine it.

## Build options

| CMake options | `cs_float` | `cs_double` |
| --- | --- | --- |
| `USE_DOUBLE=ON`, `USE_FLOAT=OFF` (default) | `double` | `double` |
| `USE_DOUBLE=OFF`, `USE_FLOAT=OFF` | `float` | `double` |
| `USE_DOUBLE=OFF`, `USE_FLOAT=ON` | `float` | `float` |

`USE_FLOAT` and `USE_DOUBLE` cannot both be enabled. For single precision:

```sh
cmake -S . -B build -G Ninja -DUSE_DOUBLE=OFF -DUSE_FLOAT=ON
ninja -C build
```

For ESP32-P4 and ESP32-S3, `CSOUND_ESP32_TARGET` selects the chip's toolchain
and single-precision defaults. See the [ESP32 build guide](../platform/esp32/README.md).

The installed `float-version.h` records the build options. Hosts and plugins
must use the headers from the library they load. Changing precision changes
the ABI; rebuild hosts and plugins when changing it. `USE_FLOAT` also reduces
the precision and range of internal calculations and time values.

Plugins built with `USE_FLOAT` mark that ABI in `csoundModuleInfo()`.
The loader checks both precisions before calling plugin entry points.
`LINKAGE`, `FLINKAGE`, their built-in variants, and `modload.h` supply the
metadata. Hand-written `csoundModuleInfo()` functions must return
`CSOUND_MODULE_INFO` from `csdl.h`.

The default and mixed precision modes keep their old metadata values.
Plugins with missing or partial metadata still use the legacy 64-bit
`cs_double` ABI; `USE_FLOAT` engines reject them. Rebuild those plugins
with the new headers and matching options to use them with `USE_FLOAT`.
Older native loaders also reject plugins that carry the new `USE_FLOAT` flag.

The aliases control Csound storage and interfaces. They do not change C's
promotion rules, floating-point literals, or external library interfaces.
Fixed-width formats such as ATS, SDIF, and OSC still use 64-bit `double`
where their formats require it. External libraries such as yyjson also keep
their required types. A few envelopes and accumulators keep `double` where
single precision loses small updates. Bundled third-party code keeps its own
types.

## Source check

The `Numeric type policy` workflow checks every pull request. Run the same
check locally with Python 3 and Git, without building Csound.

```sh
python3 scripts/check_numeric_types.py
```

The check covers tracked C and C++ files, headers, header templates, bindings,
and parser and lexer inputs. Test suites and build probes are excluded.
It flags `MYFLT` and plain `double`, including
casts, macros and code for other platforms. It ignores comments and string
literals. Plain `float` remains valid for fixed 32-bit buffers and interfaces.
The check does not decide whether a value belongs in `cs_float` or `cs_double`,
or whether an external interface needs a fixed-width type. Those choices still
need code review.

When a line must keep one of the flagged types, put an ignore comment directly
above it and explain why.

```c
/* csound-numeric-ignore double: libsndfile requires a double buffer. */
double peak;
```

Both `/* */` and `//` comments work. An ignore covers only the named type on
the next logical line. For a macro continued with backslashes, put the comment
above the `#define`. The whole continued macro counts as one logical line.
List both types as `double, MYFLT` only if the line needs both. Missing reasons,
unknown type names and unused ignores fail the check. Remove an ignore when
its code no longer needs it.

Whole files or directories can be excluded in
[`scripts/numeric_type_exclusions.json`](../scripts/numeric_type_exclusions.json).
Each entry needs a reason. Directory paths end with `/`. Use these exclusions
for sources such as bundled libraries that must keep their own types. Use
comments for exceptions within Csound code so the rest of each file stays
checked. Exclusions that no longer match a tracked source file also fail.

## Helper names and bindings

Use `CS_FLOAT2LONG`, `CS_FLOAT2LRND`, `CS_FLOAT2LONG64`, `CS_FLOAT2LRND64`,
`CS_FLOAT2UINT64`, `CS_FLOAT_INT_TYPE`, and `csoundUndenormalizeCsFloat`
in place of their `MYFLT` names. The old names remain aliases for CS7.
The existing platform limits on these helpers still apply.

`csoundGetSizeOfCsFloat()` and `csoundGetSizeOfCsDouble()` return the sizes
used by the loaded library. `csoundGetSizeOfMYFLT()` remains available.
The plugin API exposes `GetSizeOfCsFloat` and its old name `GetSizeOfMYFLT`
in the same function-pointer slot.

Python exposes `cs_float` and `cs_double` as ctypes types and keeps `MYFLT`
as an alias. The binding gets both sizes from the loaded library. The Java
wrapper keeps the `CsoundMYFLTArray` class name for source compatibility.

For C functions that write to a `cs_double` pointer, use `cs_modf` in place
of `modf` and `"%" CS_DOUBLE_SCAN` in scanf formats. These select the matching
single- or double-precision function or format.
