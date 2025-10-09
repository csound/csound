{ pkgs, pkgsWasm }:

let lib = pkgs.lib;
    stdenvWasm = pkgsWasm.clang17Stdenv;

in stdenvWasm.mkDerivation rec {
    name = "libogg";
    src = pkgs.libogg.src;
    nativeBuildInputs = [
      pkgs.cmake
      pkgs.pkg-config
    ];
}
