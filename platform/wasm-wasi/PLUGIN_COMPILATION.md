# Csound WASM Plugin Compilation

This document explains how to compile Csound WASM plugins without rebuilding Csound from source for every plugin change.

## What Changed

The WASM build now produces an SDK archive:

- `lib/csound-plugin-sdk.tar.gz`

The archive contains:

- `include/csound/*` headers
- `lib/libcsound64.a` (for SDK completeness)

Plugin derivations (`plugin_example.nix`, `plugin_example_cpp.nix`) are configured to use this SDK archive by default.

## Why This Helps

When iterating on plugins, rebuilding full Csound is expensive.

With the SDK archive, plugin builds only need:

1. WASI toolchain
2. Csound headers from the archive

This significantly reduces iteration time for plugin development.

## Build Flow

From `platform/wasm-wasi/`:

```sh
bash ./scripts/compile.sh
```

This does:

1. Build the browser module and SDK (`result`)
2. Build the standalone command module (`result_cli`)
3. Build C and C++ plugin examples against the SDK archive
4. Copy all outputs to `platform/wasm-wasi/lib/`

Expected outputs:

- `lib/csound.wasm`
- `lib/csound.wasm.z`
- `lib/csound-cli.wasm`
- `lib/csound-plugin-sdk.tar.gz`
- `lib/plugin_example.wasm`
- `lib/plugin_example_cpp.wasm`

## Building a Plugin Against an SDK Archive

You can point plugin builds at any SDK archive path (including one distributed externally).

Example:

```sh
source ./scripts/nixpkgs-pin.sh
nix-build -E '(with import <nixpkgs> {}; pkgs.callPackage ./src/plugin_example_cpp.nix {
  csoundSdkArchive = /absolute/path/to/csound-plugin-sdk.tar.gz;
  useSdkArchive = true;
})'
```

## Static runtime libraries in browser plugins

Use the Csound 7 SDK and the nixpkgs revision in
[`scripts/nixpkgs-pin.sh`](scripts/nixpkgs-pin.sh). That revision provides
wasi-libc SDK 27 and Clang 21. Build side modules and their static dependencies
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
the engine need not export these internal names. SDK 27 defines them as aliases
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
tag ABI depend on the toolchain version. SDK 27 uses the `C_LONGJMP` tag without
a Clang-version branch; recheck newer source and compiler versions together.
Use `--allow-undefined` when linking side modules that import exception tags, and
export `__wasm_apply_data_relocs` when present. Each plugin gets private
`__c_longjmp` and `__cpp_exception` tags for its statically linked runtime.
Exceptions cannot be caught across plugins with separate runtimes. Shared
dynamic-library dependencies remain unsupported.

## Fallback Mode (Rebuild from Source)

If needed, disable SDK usage:

```sh
source ./scripts/nixpkgs-pin.sh
nix-build -E '(with import <nixpkgs> {}; pkgs.callPackage ./src/plugin_example_cpp.nix {
  useSdkArchive = false;
})'
```

This falls back to `src/csound.nix`.

## Distribution for Other Users

Yes, the SDK archive can be distributed.

A user with:

- `csound-plugin-sdk.tar.gz`
- WASI toolchain (via Nix in this setup)

can compile compatible plugins without building Csound itself.
