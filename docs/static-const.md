# Static functions and const inputs

Give a function `static` linkage when only its source file uses it. Put a
declaration in a shared header when other files need it. Keep public API
functions and plugin entry points externally visible.

Use `const` for pointer parameters when the function only reads the pointed-to
data. A parameter such as `const cs_float *input` tells callers that the
function will not write through that pointer. Adding `const` to a scalar
parameter passed by value does not make the caller's data safer.

## Run the checks

The `Static and const correctness` workflow fails on violations. It uses
clang-tidy for read-only pointer parameters and C++ internal linkage. A small
libclang check covers C functions and checks references across the repository.
clang-format cannot check these rules.

On Ubuntu 24.04, install `clang-19`, `clang-tidy-19` and `python3-clang-19`.
Configure and build Csound with Clang to create `compile_commands.json` and
the parser sources, then run

```sh
CLANG=clang-19 CLANG_TIDY=clang-tidy-19 /usr/bin/python3 tests/test_static_const.py
/usr/bin/python3 scripts/check_static_const.py -p build \
  --clang clang-19 --clang-tidy clang-tidy-19
```

Keep libclang, its Python bindings and clang-tidy on the same major version.
The runner accepts `--libclang` or `LIBCLANG_LIBRARY_FILE` when the library
is outside the system search path.

The scan uses tracked implementation files in the build's compilation
database. It checks all of those files on each run, so a changed header can
expose a problem in an unchanged source file. C linkage checks also read
references from other tracked files, including tests and other platforms.
They leave functions with such references alone, even if the current build
does not compile the caller.

## Files outside the scan

The runner excludes tests, examples, generated build files, `third_party/`
and other directories outside the first-party source roots. It also skips
these embedded dependencies and generated sources

- `InOut/libmpadec/`
- `InOut/alphanumcmp.c`
- `util/SDIF/`
- `OOps/pffft.c`
- `Opcodes/tl/fractalnoise.cpp`

These files may still supply references or declarations. The linter never
rewrites them, and diagnostics from included headers are disabled.
Nested `vendor`, `external`, `third_party`, `node_modules`, `vcpkg`, `tests`
and `examples` directories are also excluded.

## Exceptions and limits

Callback signatures and public function tables can require writable pointer
types. Some atomic reads also need writable pointers on Windows. Keep these
types when needed and explain why above a
`NOLINTNEXTLINE(readability-non-const-parameter)` comment.

For a C function that a host finds by name without a shared declaration, put
`csound-linkage-ignore: <reason>` in the comment immediately above its
definition. Prefer a real interface declaration or export attribute when one
fits. Macro-generated registration entry points are exempt.

The checks are conservative. Names in inactive code and strings count as
possible external references. Unused C functions need a separate dead-code
review. Const analysis does not prove every function
read-only, and the workflow covers the enabled Linux build configuration.
Other platform branches and API signature changes still need review.
