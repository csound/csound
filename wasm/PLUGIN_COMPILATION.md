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

From `wasm/`:

```sh
bash ./scripts/compile.sh
```

This does:

1. Build the browser module and SDK (`result`)
2. Build the standalone command module (`result_cli`)
3. Build C and C++ plugin examples against the SDK archive
4. Copy all outputs to `wasm/lib/`

Expected outputs:

- `lib/csound.wasm`
- `lib/csound-no-entry.wasm`
- `lib/csound-no-entry.wasm.z`
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
