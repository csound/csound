{ system ? builtins.currentSystem, pkgs ? import <nixpkgs> {inherit system;}, moduleKind ? "browser", gitHash ? "", buildDate ? "" }:
let
  adapterSource = pkgs.lib.sourceFilesBySuffices ../scripts/wasm64 [".py"];
  toolchain = import ./wasi64-toolchain.nix {inherit system pkgs;};
  base = import ./csound.nix {
    inherit system pkgs moduleKind gitHash buildDate;
    wasiToolchain = toolchain;
  };
in base.overrideAttrs (old: {
  pname = "csound-wasm64-${moduleKind}";
  cmakeFlags = builtins.filter (f: f != "-DUSE_DOUBLE=0" && f != "-DUSE_FLOAT=1") old.cmakeFlags
    ++ ["-DUSE_DOUBLE=1" "-DUSE_FLOAT=0"];
  NIX_LDFLAGS = old.NIX_LDFLAGS ++ ["--fatal-warnings" "--import-memory" "--export-memory" "--export=cabi_realloc" "-lsetjmp"];
  nativeBuildInputs = old.nativeBuildInputs ++ [pkgs.python3];
  postInstall = if moduleKind == "browser" then ''
    mkdir -p "$out/lib"
    python3 ${adapterSource}/bridge.py "$out/bin/csound" "$out/lib/csound_x64.wasm" --browser
    node --experimental-wasm-exnref ../platform/wasm-wasi/tests/check-precision.mjs "$out/lib/csound_x64.wasm"
    mkdir -p "$out/share/csound-plugin-sdk/include" "$out/share/csound-plugin-sdk/lib"
    cp -R "$out/include/csound" "$out/share/csound-plugin-sdk/include/"
    cp "$out/lib/libcsound64.a" "$out/share/csound-plugin-sdk/lib/"
    tar -C "$out/share" -czf "$out/lib/csound_x64-plugin-sdk.tar.gz" csound-plugin-sdk
  '' else ''
    python3 ${adapterSource}/bridge.py "$out/bin/csound" "$out/lib/csound_x64-cli.wasm"
    for tool in "$out/bin/"*; do
      name=$(basename "$tool")
      if [ "$name" != csound ]; then
        python3 ${adapterSource}/bridge.py "$tool" "$out/lib/''${name}_x64.wasm"
      fi
    done
    run_tool() {
      local tool="$1"
      shift
      wasmtime run -Wexceptions=y -Ccache=n --dir=. "$out/lib/''${tool}_x64.wasm" "$@"
    }
    printf 'i1 m1 b1 C4 q mf\n' | run_tool csbeats >beats.sco
    printf 'i 1 0 1\ne\n' | run_tool scsort >sorted.sco
    run_tool mkir -g -t0.01 sweep.wav
    test -s sweep.wav
    run_tool src_conv -r22050 -oconverted.wav sweep.wav
    test -s converted.wav
  '';
  postFixup = pkgs.lib.optionalString (moduleKind == "browser") ''
    zopfli --zlib -c "$out/lib/csound_x64.wasm" > "$out/lib/csound_x64.wasm.z"
  '';
})
