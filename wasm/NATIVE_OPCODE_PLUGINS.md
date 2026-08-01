# Native Csound WebAssembly plugins

Native Csound can load supported WebAssembly opcode plugins through Wasmtime.
Pass the raw WebAssembly file with the same option used for native opcode
libraries:

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
version no newer than the host, and the same `MYFLT` size. The module must
contain its `OPCODE.WASM` marker and the fixed loader layout used by the browser
build. It supports:

- 64-bit `MYFLT` values
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
loader supplies only the documented 256-byte `INSDS` prefix. Guest code must
not read later `INSDS` fields. A change to `OENTRY`, `OPDS`, `INSDS`, `CS_TYPE`,
or the bridged part of `CSOUND` needs a versioned ABI change in the header,
browser loader, native loader, and plugin compiler.

## Build support

CMake looks for the Wasmtime C API during a native build. When it finds
Wasmtime, it builds WebAssembly opcode support. When it does not, Csound builds
without that support.

Point CMake at an unpacked or installed Wasmtime C API package if it is outside
the normal search paths:

```sh
cmake -S . -B build -DWasmtime_ROOT=/path/to/wasmtime
```

The root must contain Wasmtime 47 or newer with the compiler, cache, and GC
reference C APIs. An AOT-only Wasmtime build cannot load the raw plugin format.
To disable the feature even when CMake can find Wasmtime, set:

```sh
cmake -S . -B build -DUSE_WASMTIME=OFF
```

`USE_WASMTIME` does not download, install, or bundle Wasmtime. A shared Csound
build links to the Wasmtime library that CMake found. Keep that library in the
system loader path, or include it and its licence in your Csound package. Set
`USE_WASMTIME=OFF` when a package should not depend on Wasmtime.

A static Csound build also exposes Wasmtime as a link dependency. CMake users
need the matching Wasmtime C API package when they configure and link a static
consumer. If it is absent, the installed Csound package still exposes its
shared target but leaves out `Csound::Csound-static`.

## Compile cache

Plugin authors ship one raw `.wasm` file. They do not need to make a separate
file for each CPU or operating system.

On the first load, Wasmtime validates and compiles that file. Wasmtime stores
the compiled code in its internal, per-user cache. On later loads, it can find
the same cache entry and skip that work. This cuts later start time; it does
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

CMake finds `wasm32-wasi-clang` or `wasm32-unknown-wasi-clang` when either is
on `PATH`. You can also set the compiler and its sysroot:

```sh
cmake -S . -B build \
  -DCSOUND_WASM_TEST_C_COMPILER=/path/to/clang \
  -DCSOUND_WASM_TEST_SYSROOT=/path/to/wasi-sysroot
```

When `WASI_SDK_PATH` is set, CMake uses the Clang and sysroot in that SDK.
