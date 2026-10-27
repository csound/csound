{ system ? builtins.currentSystem, pkgs ? import <nixpkgs> {inherit system;} }:
let toolchain = import ./wasi-preview2-toolchain.nix {inherit system pkgs;};
in toolchain.stdenv.mkDerivation {
  pname = "csound-wasi-preview2-support";
  version = "1";
  dontUnpack = true;
  nativeBuildInputs = [pkgs.wasm-tools pkgs.wasmtime pkgs.nodejs];
  buildPhase = ''
    $CC ${../tests/preview2-host.c} -o probe.core.wasm
    wasm-tools component new probe.core.wasm -o probe.wasm
    mkdir files-root
    wasmtime run -Ccache=n --dir=files-root::. probe.wasm preview2
    node ${../tests/check-preview2-host.mjs} probe.core.wasm ${../browser/src}
  '';
  installPhase = ''mkdir -p "$out"; cp probe*.wasm "$out/"'';
}
