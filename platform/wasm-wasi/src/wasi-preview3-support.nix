{ system ? builtins.currentSystem, pkgs ? import <nixpkgs> {inherit system;} }:
let toolchain = import ./wasi-preview3-toolchain.nix {inherit system pkgs;};
in toolchain.stdenv.mkDerivation {
  pname = "csound-wasi-preview3-support";
  version = "1";
  dontUnpack = true;
  nativeBuildInputs = [pkgs.wasm-tools pkgs.wasmtime pkgs.nodejs];
  buildPhase = ''
    $CC ${../tests/preview3-host.c} -Wl,--export=free -o probe.core.wasm
    wasm-tools component new probe.core.wasm -o probe.wasm
    mkdir -p files-root/project
    printf 'abc' | wasmtime run -S p3 -S cwd=/project -Ccache=n --dir=files-root::/ probe.wasm preview3
    node ${../tests/check-preview3-host.mjs} probe.core.wasm ${../browser/src}
  '';
  installPhase = ''mkdir -p "$out"; cp probe*.wasm "$out/"'';
}
