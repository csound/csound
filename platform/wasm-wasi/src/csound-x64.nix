{ system ? builtins.currentSystem, pkgs ? import <nixpkgs> {inherit system;}, ... }:
throw ''
  csound_x64 requires wasm64-wasip2: 64-bit pointers and USE_DOUBLE=1.
  wasi-libc and the Component Model tools do not yet provide that target.
  Run src/wasi64-support.nix to check the host's memory64 canonical ABI.
  No wasm32 or Preview 1 substitute is built under this name.
''
