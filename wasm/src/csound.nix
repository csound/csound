{
  pkgs ? import <nixpkgs> { },
  static ? false,
  gitHash ? "HEAD",
}:

let
  lib   = pkgs.lib;
  llvm  = pkgs.llvmPackages_latest;

  pkgsWasm = pkgs.pkgsCross.wasi32;
  stdenvWasm = pkgsWasm.clang17Stdenv;

  exports = with builtins; (fromJSON (readFile ./exports.json));
  libsndfile = pkgs.callPackage ./libsndfile.nix { inherit pkgs pkgsWasm; };
  libogg = pkgs.callPackage ./libogg.nix { inherit pkgs pkgsWasm; };
  libvorbis = pkgs.callPackage ./libvorbis.nix { inherit pkgs pkgsWasm; };
  liblame = pkgs.callPackage ./liblame.nix { inherit pkgs pkgsWasm; };

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

  # postUnpack = "ls source; ls source/Frontends; ls source/Frontends/csound; exit 1";

  # Tools needed at build time
  nativeBuildInputs = [
    pkgs.flex
    pkgs.bison
    pkgs.cmake
  ];

  buildInputs = [
    libsndfile
    libogg
    libvorbis
  ];

  cmakeFlags = [
    "-DBUILD_DIR=build"
    "-DUSE_IPMIDI=OFF"
    "-DUSE_STATIC_DEPS=ON"
    "-DLAME_LIBRARY=${liblame}/lib/libmp3lame.a"
    "-DVorbiss_INCLUDE_DIR=${libvorbis}/include"
  ];

  NIX_CFLAGS_COMPILE = [
    "-D_WASI_EMULATED_SIGNAL=1"
    "-D_WASI_EMULATED_PROCESS_CLOCKS=1"
    "-mllvm -wasm-enable-sjlj"
  ];

  NIX_LDFLAGS = [
    "-lwasi-emulated-signal"
    "-lwasi-emulated-process-clocks"
    "-L${libvorbis}/lib/libvorbis.a"
  ];

}
