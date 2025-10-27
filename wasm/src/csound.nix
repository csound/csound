{
  pkgs ? import <nixpkgs> { },
  static ? false,
  gitHash ? "HEAD",
}:

let
  lib   = pkgs.lib;
  llvm  = pkgs.llvmPackages_latest;

  pkgsWasm = pkgs.pkgsCross.wasi32;
  stdenvWasm = pkgsWasm.clangStdenv;

  exports = with builtins; (fromJSON (readFile ./exports.json));
  libsndfile = pkgs.callPackage ./libsndfile.nix { inherit pkgs pkgsWasm stdenvWasm; };
  libogg = pkgs.callPackage ./libogg.nix { inherit pkgs pkgsWasm stdenvWasm; };
  libvorbis = pkgs.callPackage ./libvorbis.nix { inherit pkgs pkgsWasm stdenvWasm; };
  liblame = pkgs.callPackage ./liblame.nix { inherit pkgs pkgsWasm stdenvWasm; };
  libFLAC = pkgs.callPackage ./libflac.nix { inherit pkgs pkgsWasm stdenvWasm; };
  libopus = pkgs.callPackage ./libopus.nix { inherit pkgs pkgsWasm stdenvWasm; };

  gitignoreSrc = pkgs.fetchFromGitHub {
    owner = "hercules-ci";
    repo = "gitignore.nix";
    rev = "637db329424fd7e46cf4185293b9cc8c88c95394";
    hash = "sha256-HG2cCnktfHsKV0s4XW83gU3F57gaTljL9KNSuG6bnQs=";
  };
  inherit (import gitignoreSrc { inherit (pkgs) lib; }) gitignoreSource;

in
stdenvWasm.mkDerivation rec {
  pname = "csound-wasm";
  version = "0.0.0";
  src = gitignoreSource ../..;

  # Tools needed at build time
  nativeBuildInputs = with pkgs; [
    flex
    bison
    cmake
    zopfli
    pkg-config
  ];

  buildInputs = [
    libsndfile
    libogg
    libvorbis
  ];

  # enableParallelBuilding = false;

  cmakeFlags = [
    "-DBUILD_DIR=build"
    "-DUSE_IPMIDI=OFF"
    "-DBUILD_SHARED_LIBS=OFF"
    "-DBUILD_STATIC_LIBRARY=ON"
    "-DCUSTOM_MALLOC=ON"
    # Skip util libraries that try to build .so
    "-DBUILD_UTILITIES=OFF"
    # Skip deprecated opcodes that try to build .so
    "-DBUILD_DEPRECATED_OPCODES=OFF"
  ];

  NIX_CFLAGS_COMPILE = [
    "-D_WASI_EMULATED_SIGNAL=1"
    "-D_WASI_EMULATED_PROCESS_CLOCKS=1"
    "-mllvm -wasm-enable-sjlj"
  ];

  # NIX_CFLAGS_LINK = [ "-nostartfiles" ];

  NIX_LDFLAGS = [
    "-lwasi-emulated-signal"
    "-lwasi-emulated-process-clocks"
    "--lto-O2"
    "--no-entry"
    "--gc-sections"
    "--export-table" # plugins do the reverse and import
    "--import-memory"
    "-z,stack-size=131072"
    "${libogg}/lib/libogg.a"
    "${libFLAC}/lib/libFLAC.a"
    "${libvorbis}/lib/libvorbis.a"
    "${libvorbis}/lib/libvorbisenc.a"
    "${libogg}/lib/libogg.a"
    "${libopus}/lib/libopus.a"
    "--export=__heap_base"
    "--export=__data_end"
  ] ++ (
    builtins.map (name: "--export-if-defined=" + name)
    (builtins.fromJSON (builtins.readFile ./exports.json)));

  postInstall = ''
    # make a compressed version for the browser bundle
    zopfli --zlib -c $out/bin/csound > $out/lib/csound.wasm.z
    cp $out/bin/csound $out/lib/csound.wasm
  '';
}
