{
  pkgs ? import <nixpkgs> {},
  pkgsWasm ? pkgs.pkgsCross.wasi32,
  stdenvWasm ? pkgsWasm.clangStdenv,
  static ? false,
  gitHash ? "",
  buildDate ? "",
}: let
  lib = pkgs.lib;
  llvm = pkgs.llvmPackages_latest;

  exports = with builtins; (fromJSON (readFile ./exports.json));
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
    pname = "csound-wasm";
    version = "0.0.0";
    src = gitignoreSource ../..;

    # Tools needed at build time
    nativeBuildInputs = with pkgs; [
      buildPackages.flex
      buildPackages.bison
      cmake
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
        "-DBUILD_STATIC_LIBRARY=ON"
        "-DCUSTOM_MALLOC=ON"
        # Skip util libraries that try to build .so
        "-DBUILD_UTILITIES=OFF"
        "-DINSTALL_PYTHON_INTERFACE=OFF"
        # Skip deprecated opcodes that try to build .so
        "-DBUILD_DEPRECATED_OPCODES=OFF"
        "-DBISON_EXECUTABLE=${pkgs.buildPackages.bison}/bin/bison"
        "-DFLEX_EXECUTABLE=${pkgs.buildPackages.flex}/bin/flex"
      ]
      ++ lib.optionals (gitHash != "") [
        "-DCSOUND_GIT_HASH=${gitHash}"
      ]
      ++ lib.optionals (buildDate != "") [
        "-DCSOUND_BUILD_DATE=${buildDate}"
      ];

    NIX_CFLAGS_COMPILE = [
      "-D_WASI_EMULATED_SIGNAL=1"
      "-D_WASI_EMULATED_PROCESS_CLOCKS=1"
      "-mllvm -wasm-enable-sjlj"
    ];

    # NIX_CFLAGS_LINK = [ "-nostartfiles" ];

    NIX_LDFLAGS =
      [
        "-lwasi-emulated-signal"
        "-lwasi-emulated-process-clocks"
        "--lto-O2"
        "--no-entry"
        "--gc-sections"
        "--export-table" # plugins do the reverse and import
        "--growable-table" # required so dlinit can append plugin function pointers
        "--import-memory"
        "-z,stack-size=131072"
        "${libogg}/lib/libogg.a"
        "${libFLAC}/lib/libFLAC.a"
        "${libvorbis}/lib/libvorbis.a"
        "${libvorbis}/lib/libvorbisenc.a"
        "${libogg}/lib/libogg.a"
        "${liblame}/lib/libmp3lame.a"
        "${libmpg123}/lib/libmpg123.a"
        "${libopus}/lib/libopus.a"
        "--export=__heap_base"
        "--export=__data_end"
      ]
      ++ (
        builtins.map (name: "--export-if-defined=" + name)
        (builtins.fromJSON (builtins.readFile ./exports.json))
      );

    postInstall = ''
      # make a compressed version for the browser bundle
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
    '';
  }
