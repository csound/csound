# Native Csound WebAssembly plugins

Native Csound can load supported WebAssembly opcode plugins through Wasmtime.
Pass the raw WebAssembly file with the same option used for native opcode
libraries.

```sh
csound --opcode-lib=./velvetlp.wasm piece.csd
```

Csound reads and prepares the plugin before it compiles the orchestra. Native
opcode libraries still use the normal loader. The same option works through
`csoundSetOption()` when set before orchestra compilation.

An explicitly named `.wasm` file must load. Csound stops if it cannot read,
check, compile, or start the plugin. This differs from the delayed warning used
for a missing native opcode library.

## Current plugin support

The first version supports plugins built for the current OPCODE.WASM wasm32
Csound plugin ABI. The plugin must use the same Csound major version, a minor
version no newer than the host, and the same `cs_float` size. The module must
contain its `OPCODE.WASM` marker and the fixed loader layout used by the browser
build. It supports the following.

- 64-bit `cs_float` values
- fixed numeric audio-rate (`a`), control-rate (`k`), and init-rate (`i`)
  arguments
- opcode init, perf, and deinit callbacks

It does not yet support strings, arrays, function tables, spectral values,
custom argument types, optional or variable arguments, or general access to
the native `CSOUND` function table. The host-call bridge covers the allocation
call used while current plugins list their opcodes. Csound reports an error for
an unsupported type or host call. The `out_count()` and `in_count()` plugin
helpers are not available in this fixed-signature ABI.

The loader does not give opcode plugins WASI file, network, clock, or other
system access. It does not set a CPU time limit. A plugin can still stall the
audio thread, so only load plugin code you trust to run there.

Csound creates one Wasmtime instance for each plugin file and shares it across
the plugin's opcode voices, as the browser build does. Calls into the same
plugin run one at a time. This keeps plugin globals and static data consistent
between native and browser builds.

This ABI is an early compatibility layer. The loader mirrors a small part of
the wasm32 Csound layout in guest memory. It does not pass native host pointers
or native host layouts to the plugin.

The frozen wasm32 offsets live in the private `Top/wasm_opcode_abi.h` header.
Browser builds check each bridged size and offset at compile time. The native
loader reserves a 256-byte `INSDS` prefix, but supplies values only for `esr`,
`ekr`, `ksmps`, `ksmps_offset`, and `ksmps_no_end`. Guest code must not use
other `INSDS` fields, even within that prefix. In particular, helpers such as
`GetLocalKcounter()` and `GetReleaseFlag()` are not supported. A change to
`OENTRY`, `OPDS`, `INSDS`, `CS_TYPE`, or the bridged part of `CSOUND` needs a
versioned ABI change in the header, browser loader, native loader, and plugin
compiler.

## Build support

Use the [Wasmtime C API package](https://docs.wasmtime.dev/c-api/), which contains
headers and a native library. Installing the `wasmtime` command alone is not
enough. Choose the C API release asset for the machine running native Csound,
such as `x86_64-linux` or `aarch64-macos`. The plugin itself still uses wasm32.

CI uses Wasmtime **48.0.5** from the 48 LTS release line. Prefer the latest patch
in that line when packaging Csound. Wasmtime supports LTS releases for two
years, while regular releases receive fixes for two months. See the
[release policy](https://docs.wasmtime.dev/stability-release.html) and
[release downloads](https://github.com/bytecodealliance/wasmtime/releases).

For Linux on x86_64, these commands download the pinned C API package and check
its SHA-256 digest. Run them from the Csound source directory after installing
the normal [build dependencies](../../BUILD.md).

```sh
mkdir -p .deps
curl --fail --location --retry 3 \
  -o .deps/wasmtime.tar.xz \
  https://github.com/bytecodealliance/wasmtime/releases/download/v48.0.5/wasmtime-v48.0.5-x86_64-linux-c-api.tar.xz
printf '%s  %s\n' \
  81fffe5fe895c7f8f84a744f4d4d165dda28793d00ee943ee71fcfb6ab5ed923 \
  .deps/wasmtime.tar.xz | sha256sum --check
tar -xJf .deps/wasmtime.tar.xz -C .deps
cmake -S . -B build -G Ninja \
  -DWasmtime_ROOT="$PWD/.deps/wasmtime-v48.0.5-x86_64-linux-c-api"
ninja -C build
```

On macOS or Windows, unpack the matching `c-api` release asset and point
`Wasmtime_ROOT` at that directory. It must contain `include/wasmtime.h` and the
library under `lib`. Do not point it at the Wasmtime command or its directory.

CMake should print `Using Wasmtime 48.0.5 for WebAssembly opcode libraries`.
Check this message because `USE_WASMTIME=ON` allows a build without the feature
when no compatible C API package is found. The API check accepts version 47
or newer with compiler, cache, and GC reference support. This is an API
minimum, not a recommendation to use an unsupported release. An AOT-only
Wasmtime build cannot load raw plugins.

To disable the feature even when CMake can find Wasmtime, use this option.

```sh
cmake -S . -B build -DUSE_WASMTIME=OFF
```

`USE_WASMTIME` does not download, install, or bundle Wasmtime. Keep a shared
Wasmtime library in the system loader path, or include it and its licence in
your Csound package. For a local Linux install, you can add the C API package's
`lib` directory to `LD_LIBRARY_PATH`. On Windows, put `wasmtime.dll` beside the
Csound executable or in a directory on `PATH`. CMake normally adds the library
path to the macOS build's runtime search paths. Installed packages must keep a
valid runtime path to `libwasmtime.dylib`.

A static Csound build also exposes Wasmtime as a link dependency. CMake users
need a compatible C API package from the same Wasmtime major release that
built Csound. Patch updates are compatible. If that package is absent, the
installed Csound package still exposes its shared target but leaves out
`Csound::Csound-static`.

Users of a Csound package with this support do not need a WASI compiler or the
Wasmtime command to load a plugin. They need the raw opcode `.wasm` file and
the runtime library supplied with that package. Running `csound-cli.wasm`
inside the Wasmtime command is a separate use described in the
[WASI build guide](README.md).

## Compile cache

Plugin authors ship one raw `.wasm` file. They do not need to make a separate
file for each CPU or operating system.

On the first load, Wasmtime validates and compiles that file. Wasmtime stores
the compiled code in its internal, per-user cache. On later loads, it can find
the same cache entry and skip that work. This cuts later start time. It does
not change the opcode's steady audio cost.

The cache key covers the raw module contents and the host details that affect
generated code, including the Wasmtime version, target, engine settings, and
CPU features. A runtime update or host change therefore creates a new entry.

The cache is only a local speed-up. Wasmtime recompiles the raw `.wasm` file
when an entry is absent, stale, corrupt, or cannot be read. A cache read or
write failure does not stop Csound from loading the plugin. Csound warns when
it cannot load the cache setup and runs without the cache. Keep the cache
writable only by its user. It is safe to clear it.

Set `CSOUND_WASMTIME_CACHE_CONFIG` to a Wasmtime cache configuration file to
choose another cache directory. Csound still falls back to an uncached compile
if Wasmtime cannot use that file. Embedded hosts can also set it through
`csoundSetGlobalEnv()`.

Csound does not accept a user-supplied `.cwasm` file through `--opcode-lib`.
Wasmtime compiled modules contain native code and are not portable or safe as
untrusted input. Csound gives Wasmtime the raw module and lets the runtime look
up its own matching cache entry.

## Build the test plugin

The command-line test builds its plugin from C source when CTest runs. It uses
the current Csound headers and checks the frozen wasm32 layout while it
compiles. The repository does not store a built test plugin.

Use [WASI SDK 34.0](https://github.com/WebAssembly/wasi-sdk/releases/tag/wasi-sdk-34),
the version pinned in CI, to build the test plugin. Choose the SDK archive for
your build machine, unpack it, and set `WASI_SDK_PATH` to its directory.

```sh
export WASI_SDK_PATH=/path/to/wasi-sdk-34.0-x86_64-linux
cmake -S . -B build -G Ninja -DBUILD_TESTS=ON \
  -DWasmtime_ROOT=/path/to/wasmtime-c-api
ninja -C build
ctest --test-dir build -R '^commandline_wasmtime_opcode_cache$' \
  --no-tests=error --output-on-failure
```

CTest builds the fixture before running the loader checks. The compiler uses
`wasm32-wasip1`, the target name supported by current WASI SDK releases. The
fixture links without the WASI runtime because this loader does not provide
WASI system calls.

Without `WASI_SDK_PATH`, CMake looks for `wasm32-wasip1-clang`,
`wasm32-wasi-clang`, or `wasm32-unknown-wasi-clang` on `PATH`. You can also set
`CSOUND_WASM_TEST_C_COMPILER` and `CSOUND_WASM_TEST_SYSROOT` to a compatible
Clang executable and WASI sysroot. The tests need a Csound build with 64-bit
`cs_float` and Wasmtime support.
