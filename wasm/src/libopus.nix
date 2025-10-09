{ pkgs, pkgsWasm }:

let lib = pkgs.lib;
    stdenvWasm = pkgsWasm.clang17Stdenv;

in stdenvWasm.mkDerivation rec {
  pname = "libopus";
  version = "1.5.2";

  src = pkgs.fetchurl {
    url = "https://downloads.xiph.org/releases/opus/opus-${version}.tar.gz";
    hash = "sha256-ZcHS94ufL7IAgsOMvkfJUa1YOTRYduRpQWEu6H+afOE=";
  };

  nativeBuildInputs = [
    pkgs.cmake
    pkgs.pkg-config
  ];

  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=RELEASE"
    "-DBUILD_SHARED_LIBS=ON"
    "-DBUILD_PROGRAMS=OFF"
    "-DOPUS_BUILD_TESTING_HELP_STR=OFF"
  ];

  doCheck = false;

}