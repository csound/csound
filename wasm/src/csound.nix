{
  system ? builtins.currentSystem,
  pkgs ? import <nixpkgs> {inherit system;},
  pkgsWasm ? pkgs.pkgsCross.wasi32,
  stdenvWasm ? pkgsWasm.clangStdenv,
  moduleKind ? "command",
  gitHash ? "",
  buildDate ? "",
}: let
  lib = pkgs.lib;
  browserHosted =
    if moduleKind == "browser"
    then true
    else if moduleKind == "command"
    then false
    else throw "csound.nix: moduleKind must be either 'command' or 'browser'";
  libsndfile = pkgs.callPackage ./libsndfile.nix {inherit pkgs pkgsWasm stdenvWasm;};
  libogg = pkgs.callPackage ./libogg.nix {inherit pkgs pkgsWasm stdenvWasm;};
  libvorbis = pkgs.callPackage ./libvorbis.nix {inherit pkgs pkgsWasm stdenvWasm;};
  libmpg123 = pkgs.callPackage ./libmpg123.nix {inherit pkgs pkgsWasm stdenvWasm;};
  liblame = pkgs.callPackage ./liblame.nix {inherit pkgs pkgsWasm stdenvWasm;};
  libFLAC = pkgs.callPackage ./libflac.nix {inherit pkgs pkgsWasm stdenvWasm;};
  libopus = pkgs.callPackage ./libopus.nix {inherit pkgs pkgsWasm stdenvWasm;};

  gitignoreSrc = pkgs.fetchFromGitHub {
    owner = "hercules-ci";
    repo = "gitignore.nix";
    rev = "637db329424fd7e46cf4185293b9cc8c88c95394";
    hash = "sha256-HG2cCnktfHsKV0s4XW83gU3F57gaTljL9KNSuG6bnQs=";
  };
  inherit (import gitignoreSrc {inherit (pkgs) lib;}) gitignoreSource;
in
  stdenvWasm.mkDerivation rec {
    pname =
      if browserHosted
      then "csound-wasm-browser"
      else "csound-wasm-cli";
    version = "0.0.0";
    src = gitignoreSource ../..;

    # Tools needed at build time
    nativeBuildInputs = with pkgs; [
      buildPackages.flex
      buildPackages.bison
      cmake
      ninja
      zopfli
      pkg-config
    ];

    buildInputs = [
      libsndfile
      libogg
      libvorbis
      libmpg123
      liblame
    ];

    enableParallelBuilding = false;

    cmakeFlags =
      [
        "-DBUILD_DIR=build"
        "-DUSE_IPMIDI=OFF"
        "-DBUILD_SHARED_LIBS=OFF"
        # WASI makes the primary Csound library static; a second static target
        # would generate the same libcsound64.a output and Ninja rejects it.
        "-DBUILD_STATIC_LIBRARY=OFF"
        "-DBUILD_CSOUND_COMMAND=ON"
        # Skip util libraries that try to build .so
        "-DBUILD_UTILITIES=OFF"
        "-DINSTALL_PYTHON_INTERFACE=OFF"
        # Skip deprecated opcodes that try to build .so
        "-DBUILD_DEPRECATED_OPCODES=OFF"
        "-DBUILD_WASI_BROWSER=${
          if browserHosted
          then "ON"
          else "OFF"
        }"
        "-DBISON_EXECUTABLE=${pkgs.buildPackages.bison}/bin/bison"
        "-DFLEX_EXECUTABLE=${pkgs.buildPackages.flex}/bin/flex"
      ]
      ++ lib.optionals (gitHash != "") [
        "-DCSOUND_GIT_HASH=${gitHash}"
      ]
      ++ lib.optionals (buildDate != "") [
        "-DCSOUND_BUILD_DATE=${buildDate}"
      ];

    NIX_CFLAGS_COMPILE =
      [
        "-D_WASI_EMULATED_SIGNAL=1"
        "-D_WASI_EMULATED_PROCESS_CLOCKS=1"
        "-mllvm -wasm-enable-sjlj"
      ]
      # Wasmtime supports the standardized exception proposal. Keep legacy EH
      # for the browser reactor until browsers enable the standard by default.
      ++ lib.optionals (!browserHosted) ["-mllvm -wasm-use-legacy-eh=false"];

    NIX_LDFLAGS =
      [
        "-lwasi-emulated-signal"
        "-lwasi-emulated-process-clocks"
        "--lto-O2"
        "--gc-sections"
        # The CLI parser and command-line tests need more linear stack than the
        # size-tuned browser reactor.
        "-z,stack-size=${
          if browserHosted
          then "131072"
          else "1048576"
        }"
        "${libogg}/lib/libogg.a"
        "${libFLAC}/lib/libFLAC.a"
        "${libvorbis}/lib/libvorbis.a"
        "${libvorbis}/lib/libvorbisenc.a"
        "${libogg}/lib/libogg.a"
        "${liblame}/lib/libmp3lame.a"
        "${libmpg123}/lib/libmpg123.a"
        "${libopus}/lib/libopus.a"
      ]
      ++ lib.optionals browserHosted [
        "--export-table" # plugins do the reverse and import
        "--growable-table" # required so dlinit can append plugin function pointers
        "--import-memory"
        "--export=__heap_base"
        "--export=__data_end"
      ]
      ++ lib.optionals browserHosted (
        builtins.map (name: "--export-if-defined=" + name)
        (builtins.fromJSON (builtins.readFile ./exports.json))
      )
      # wasi-libc provides the exception-handling setjmp implementation as a
      # separate archive. The browser build uses its JavaScript-oriented stubs.
      ++ lib.optionals (!browserHosted) ["-lsetjmp"];

    postInstall =
      if browserHosted
      then ''
        # This reactor is instantiated by the browser host.
        zopfli --zlib -c $out/bin/csound > $out/lib/csound.wasm.z
        cp $out/bin/csound $out/lib/csound.wasm

        # Build a lightweight SDK archive for compiling external WASM plugins.
        # This can be distributed so plugin builds do not need the full Csound source build.
        mkdir -p $out/share/csound-plugin-sdk/include
        mkdir -p $out/share/csound-plugin-sdk/lib
        cp -R $out/include/csound $out/share/csound-plugin-sdk/include/
        cp $out/lib/libcsound64.a $out/share/csound-plugin-sdk/lib/
        cat > $out/share/csound-plugin-sdk/README.md <<'EOF'
        Csound WASM Plugin SDK
        ======================

        This archive contains headers and static library artifacts needed to compile
        Csound plugins for WASM/WASI without rebuilding Csound from source.
        EOF
        tar -C $out/share -czf $out/lib/csound-plugin-sdk.tar.gz csound-plugin-sdk
      ''
      else ''
        # This command module owns its memory and has a real WASI _start entry.
        cp $out/bin/csound $out/lib/csound-cli.wasm
      '';
  }
