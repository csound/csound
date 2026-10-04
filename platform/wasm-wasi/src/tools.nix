{
  system ? builtins.currentSystem,
  pkgs ? import <nixpkgs> {inherit system;},
  pkgsWasm ? pkgs.pkgsCross.wasi32,
  stdenvWasm ? pkgsWasm.clangStdenv,
  gitHash ? "",
  buildDate ? "",
}: let
  lib = pkgs.lib;
  tools = [
    "atsa" "csbeats" "cvanal" "dnoise" "envext" "extract" "extractor"
    "het_export" "het_import" "hetro" "lpanal" "lpc_export" "lpc_import"
    "mixer" "mkir" "pv_export" "pv_import" "pvanal" "pvlook" "scale"
    "scot" "scsort" "sdif2ad" "smf_conv" "src_conv"
  ];
  targets = map (name: if name == "mixer" then "mixer-bin" else name) tools;
  samplerate = stdenvWasm.mkDerivation {
    pname = "libsamplerate-wasi";
    inherit (pkgs.libsamplerate) version src;
    nativeBuildInputs = [pkgs.cmake pkgs.ninja];
    cmakeFlags = [
      "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
      # This release prepends the install prefix in samplerate.pc itself.
      "-DCMAKE_INSTALL_LIBDIR=lib"
      "-DCMAKE_INSTALL_INCLUDEDIR=include"
      "-DBUILD_SHARED_LIBS=OFF"
      "-DBUILD_TESTING=OFF"
      "-DLIBSAMPLERATE_EXAMPLES=OFF"
      "-DLIBSAMPLERATE_INSTALL=ON"
    ];
  };
  csound = import ./csound.nix {
    inherit system pkgs pkgsWasm stdenvWasm gitHash buildDate;
    moduleKind = "command";
  };
in
csound.overrideAttrs (old: {
  pname = "csound-wasm-tools";
  nativeBuildInputs = old.nativeBuildInputs ++ [pkgs.wasmtime];
  buildInputs = old.buildInputs ++ [samplerate];
  cmakeFlags = lib.filter
    (flag: !(builtins.elem flag ["-DBUILD_UTILITIES=OFF" "-DBUILD_CSOUND_COMMAND=ON"]))
    old.cmakeFlags ++ [
      "-GNinja"
      "-DBUILD_UTILITIES=ON"
      "-DBUILD_CSOUND_COMMAND=OFF"
      "-DBUILD_CSBEATS=ON"
      "-DBUILD_SRC_CONV=ON"
      "-DUSE_LIBSAMPLERATE=ON"
    ];

  # Build only the requested commands, without unrelated launchers or plugins.
  buildPhase = ''
    runHook preBuild
    cmake --build . --parallel "$NIX_BUILD_CORES" --target ${lib.escapeShellArgs targets}
    runHook postBuild
  '';
  installPhase = ''
    runHook preInstall
    mkdir -p "$out/lib"
    for tool in ${lib.escapeShellArgs tools}; do
      install -m644 "$tool" "$out/lib/$tool.wasm"
    done
    runHook postInstall
  '';

  # These run on the build host, even though the binaries target wasm32.
  # Most utilities report usage with a nonzero status and have no --help mode.
  # Keep their compilation checks, and run small valid inputs where practical.
  postInstall = ''
    export HOME="$TMPDIR"
    run_tool() {
      local tool="$1"
      shift
      wasmtime run -Wexceptions=y -Ccache=n --dir=. "$out/lib/$tool.wasm" "$@"
    }
    printf 'i1 m1 b1 C4 q mf\n' | run_tool csbeats >beats.sco
    printf 'i 1 0 1\ne\n' | run_tool scsort >sorted.sco
    run_tool mkir -g -t0.01 sweep.wav
    test -s sweep.wav
    run_tool src_conv -r22050 -oconverted.wav sweep.wav
    test -s converted.wav
  '';
})
