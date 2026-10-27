{ system ? builtins.currentSystem, pkgs ? import <nixpkgs> {inherit system;} }:
let
  adapterSource = pkgs.lib.sourceFilesBySuffices ../scripts/wasm64 [".py"];
  toolchain = import ./wasi64-toolchain.nix {inherit system pkgs;};
in toolchain.stdenv.mkDerivation {
  pname = "csound-wasip2-memory64-support";
  version = "1";
  dontUnpack = true;
  nativeBuildInputs = [pkgs.wasm-tools pkgs.wasmtime pkgs.nodejs pkgs.python3];
  buildPhase = ''
    $CC ${../tests/preview2-host.c} -Wl,--export=cabi_realloc -o probe.raw.wasm
    python3 ${adapterSource}/bridge.py probe.raw.wasm probe.wasm
    mkdir files-root
    wasmtime run -Ccache=n --dir=files-root::. probe.wasm preview2
    node ${../tests/check-preview2-host.mjs} probe.core.wasm ${../browser/src}
  '';
  installPhase = ''mkdir -p "$out"; cp probe*.wasm "$out/"'';
}
