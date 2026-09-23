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

The installed `float-version.h` records the build options. Hosts and plugins
must use the headers from the library they load. Changing precision changes
the ABI; rebuild hosts and plugins when changing it. `USE_FLOAT` also reduces
the precision and range of internal calculations and time values.

The aliases control Csound storage and interfaces. They do not change C's
promotion rules, floating-point literals, or external library interfaces.
Fixed-width formats such as ATS, SDIF, and OSC still use 64-bit `double`
where their formats require it. Bundled third-party code keeps its own types.

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
