# Csound WASM Plugin Compilation

This document explains how to compile Csound WASM plugins without rebuilding Csound from source for every plugin change.

## Choose the target

The build supplies two Csound SDK archives. Neither is a compiler SDK bundle.
They contain generated headers and the matching static Csound library.

| Engine | SDK | Plugin build option |
| --- | --- | --- |
| wasm32 float `csound.wasm` | `csound-plugin-sdk.tar.gz` (`libcsound.a`) | `--arg wasm64 false` |
| wasm64 double `csound-wasm64.wasm` | `csound-wasm64-plugin-sdk.tar.gz` (`libcsound64.a`) | `--arg wasm64 true` |

The default browser uses wasm32 with float samples. The opt-in
`@csound/browser/wasm64.js` module needs wasm64 plugins with double samples. The loader rejects a plugin
with a different pointer or sample size.
Let the SDK's generated `float-version.h` set the precision macros. Do not
pass `-DUSE_DOUBLE=0` to the compiler; Csound checks whether it is defined.

## Build

From `platform/wasm-wasi`, `npm run build` creates both engines, commands,
SDKs and C/C++ example plugins. To rebuild only a wasm64 plugin:

```sh
source scripts/nixpkgs-pin.sh
nix-build src/plugin_example.nix --arg wasm64 true
nix-build src/plugin_example_cpp.nix --arg wasm64 true
```

The outputs are `plugin_example-wasm64.wasm` and `plugin_example_cpp-wasm64.wasm`.
For an external archive, also pass
`--arg csoundSdkArchive /absolute/path/to/csound-wasm64-plugin-sdk.tar.gz`.
`--arg useSdkArchive false` builds the matching engine's headers from source.

The example builds use the source-built wasm64 Preview 2 toolchain and append
`csound.abi` metadata with `scripts/wasm64/wasm.py`. External builds must do the
same so the loader can read native signatures and pointer widths. The engine
provides its Preview 2 imports to plugins. A plugin with additional WASI imports
can embed its own adapter with `scripts/wasm64/bridge.py --browser`. The loader
connects that adapter to the engine's allocator and memory and shares its
filesystem. Each adapter keeps its own WASI resource handles.
Shared-library dependencies remain unsupported.

## Static runtime libraries in browser plugins

Use the Csound 7 SDK and the nixpkgs revision in
[`scripts/nixpkgs-pin.sh`](scripts/nixpkgs-pin.sh). That revision provides
wasi-libc SDK 32 and Clang 21. Build side modules and their static dependencies
as position-independent code (`-fPIC`). Import the engine's memory, table, and
stack pointer.

For both direct function imports and `GOT.func` addresses, the loader uses the
engine's allocator and `memcpy`, `memmove`, and `memset`. Other symbols use the
plugin's own export when it defines a strong symbol. A weak definition uses an
engine export when one exists, then falls back to the plugin's own export. The
loader preserves direct function imports that the caller already supplied. It fills
`GOT.func` and `GOT.mem` address imports before running data relocations and
constructors. Missing weak addresses stay zero; missing strong symbols fail
loading.

All plugins share the engine's allocator. The browser engine exports `malloc`,
`calloc`, `realloc`, `free`, `aligned_alloc`, and `posix_memalign`. The loader maps
`__libc_malloc`, `__libc_calloc`, and `__libc_free` to those public Wasm functions;
the engine need not export these internal names. SDK 32 defines them as aliases
with the same signatures. The engine still exports
`__wasilibc_find_relpath_alloc` so plugin file operations use its working
directory. This internal function has no public alias. Recheck its name and
signature when updating the toolchain; a missing symbol stops the engine build.

The [C example](src/plugin_example.nix) links with `-nostdlib`, imports its libc
functions from the engine, and includes only its own PIC setjmp support. Plugins
that need other libc routines must build a private PIC WASI libc with
`MALLOC_IMPL=none` so they do not link a second allocator into shared memory.
The [plugins Nix build](https://github.com/hlolli/plugins/blob/cdc51c0842aa17c78a284b64d059cfb4e6950730/wasm/default.nix)
uses this path for the image, Chua, and linear algebra plugins. It builds the
private libc and C++ runtime from the same nixpkgs pin and passes their archives
to CMake.

For WASI `setjmp`/`longjmp`, compile with `-mllvm -wasm-enable-sjlj` and link PIC
setjmp support. The C example compiles the pinned wasi-libc source at
`libc-top-half/musl/src/setjmp/wasm32/rt.c`. Both the source path and the exception
tag ABI depend on the toolchain version. SDK 32 uses the `C_LONGJMP` tag without
a Clang-version branch; recheck newer source and compiler versions together.
Use `--allow-undefined` when linking side modules that import exception tags, and
export `__wasm_apply_data_relocs` when present. Each plugin gets private
`__c_longjmp` and `__cpp_exception` tags for its statically linked runtime.
Exceptions cannot be caught across plugins with separate runtimes. Shared
dynamic-library dependencies remain unsupported.
