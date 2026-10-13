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
nix-build -E '(with import <nixpkgs> {}; pkgs.callPackage ./src/plugin_example_cpp.nix {
  csoundSdkArchive = /absolute/path/to/csound-plugin-sdk.tar.gz;
  useSdkArchive = true;
})'
```

## Static runtime libraries in browser plugins

Build side modules and their static dependencies as position-independent code
(`-fPIC`). Import the engine's memory, table, and stack pointer. The loader binds
weak function definitions to an engine export when one exists, or to the plugin's
own export. It also fills `GOT.func` and `GOT.mem` address imports before running
data relocations and constructors. Missing weak addresses stay zero; missing
strong symbols fail loading.

All plugins share the engine's allocator. The browser engine exports `malloc`,
`calloc`, `realloc`, `free`, `aligned_alloc`, `posix_memalign`, the corresponding
WASI libc allocation aliases, and `memcpy`, `memmove`, and `memset`. Build a private
WASI libc with `MALLOC_IMPL=none` to avoid linking a second allocator into shared
memory. The engine also exports `__wasilibc_find_relpath_alloc` so plugin file
operations use its working directory.

For WASI `setjmp`/`longjmp`, compile with `-mllvm -wasm-enable-sjlj` and link PIC
setjmp support. The C example does this with the toolchain's unmodified source.
Use `--allow-undefined` when linking side modules that import exception tags, and
export `__wasm_apply_data_relocs` when present. Each plugin gets private
`__c_longjmp` and `__cpp_exception` tags for its statically linked runtime.
Exceptions cannot be caught across plugins with separate runtimes. Shared
dynamic-library dependencies remain unsupported.

## Fallback Mode (Rebuild from Source)

If needed, disable SDK usage:

```sh
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
