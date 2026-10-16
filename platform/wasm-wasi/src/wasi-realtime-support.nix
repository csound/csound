{
  system ? builtins.currentSystem,
  pkgs ? import <nixpkgs> { inherit system; },
  stdenvWasm ? (import ./wasi-realtime-toolchain.nix { inherit pkgs; }).stdenv,
}:
let maxMemory = builtins.fromJSON (builtins.readFile ./shared-memory-max.txt);
in stdenvWasm.mkDerivation {
  pname = "csound-wasi-realtime-support";
  version = "1";
  dontUnpack = true;
  dontConfigure = true;
  dontFixup = true;
  buildPhase = ''
    runHook preBuild
    # Use the same compile/link probe as CMake. A failed capability probe
    # selects the baseline artifact; unrelated Nix build failures still fail.
    if $CC -pthread -Wl,--import-memory,--export-memory,--max-memory=${toString maxMemory} \
      ${../../../cmake/check_wasi_realtime.c} -o probe.wasm > probe.log 2>&1; then
      echo 1 > supported
    else
      echo 0 > supported
    fi
    runHook postBuild
  '';
  installPhase = ''
    mkdir -p "$out"
    cp supported probe.log "$out/"
  '';
}
