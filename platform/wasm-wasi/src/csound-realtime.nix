{
  system ? builtins.currentSystem,
  pkgs ? import <nixpkgs> { inherit system; },
  gitHash ? "",
  buildDate ? "",
}:
let
  toolchain = import ./wasi-realtime-toolchain.nix { inherit pkgs; };
  base = import ./csound.nix {
    inherit system pkgs gitHash buildDate;
    stdenvWasm = toolchain.stdenv;
    moduleKind = "command";
  };
in base.overrideAttrs (old: {
  pname = "csound-wasi-realtime";
  cmakeFlags = old.cmakeFlags ++ [
    "-DBUILD_WASI_REALTIME=ON"
    "-DBUILD_UTILITIES=OFF"
    "-DBUILD_CSBEATS=OFF"
    "-DBUILD_SRC_CONV=OFF"
    "-DBUILD_MULTI_CORE=OFF"
  ];
  nativeBuildInputs = old.nativeBuildInputs ++ [ pkgs.python3 ];
  # Each dependency also uses this compiler and its threaded C/C++ runtime.
  buildPhase = ''
    runHook preBuild
    ninja -j"$NIX_BUILD_CORES" csound-bin wasi-pthread-smoke wasi-realtime-smoke
    # Cross builds skip the normal check phase; this host runs Wasmtime.
    python3 ../platform/wasm-wasi/tests/realtime/run.py \
      --wasmtime "${pkgs.wasmtime}/bin/wasmtime" --build-dir .
    runHook postBuild
  '';
  installPhase = ''
    runHook preInstall
    mkdir -p "$out/bin" "$out/lib" "$out/include/csound"
    install -m755 csound "$out/bin/csound"
    cp csound "$out/lib/csound-realtime.wasm"
    cp libcsound64.a "$out/lib/"
    cp ../include/*.h include/*.h "$out/include/csound/"
    runHook postInstall
  '';
  postInstall = "";
  passthru = (old.passthru or {}) // { inherit toolchain; };
})
