{ system ? builtins.currentSystem, pkgs ? import <nixpkgs> {inherit system;} }:
let
  adapterSource = pkgs.lib.sourceFilesBySuffices ../scripts/wasm64 [".py"];
  toolchain = import ./wasi64-toolchain.nix {inherit system pkgs;};
in toolchain.stdenv.mkDerivation {
  pname = "csound-wasip3-memory64-support";
  version = "1";
  dontUnpack = true;
  nativeBuildInputs = [pkgs.wasm-tools pkgs.wasmtime pkgs.nodejs pkgs.python3];
  buildPhase = ''
    $CC ${../tests/preview3-host.c} -Wl,--export=cabi_realloc -Wl,--export=free -o probe.raw.wasm
    python3 ${adapterSource}/bridge.py probe.raw.wasm probe.wasm
    mkdir files-root
    printf 'abc' | wasmtime run -S p3 -Ccache=n --dir=files-root::. probe.wasm preview3
    node ${../tests/check-preview3-host.mjs} probe.core.wasm ${../browser/src}
    wasm-tools parse ${../tests/preview3-bridge-engine.wat} -o bridge-engine.wasm
    node ${../tests/check-preview3-bridge.mjs} probe.core.wasm bridge-engine.wasm ${../browser/src}
  '';
  installPhase = ''mkdir -p "$out"; cp probe*.wasm "$out/"'';
}
