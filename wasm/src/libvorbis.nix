{ pkgs, pkgsWasm }:

let lib = pkgs.lib;
    stdenvWasm = pkgsWasm.clang17Stdenv;
    libogg = pkgs.callPackage ./libogg.nix { inherit pkgs pkgsWasm; };

in stdenvWasm.mkDerivation rec {
    name = "libvorbis";
    src = pkgs.libvorbis.src;
    nativeBuildInputs = [
      pkgs.cmake
      pkgs.pkg-config
    ];

    NIX_CFLAGS_COMPILE = [
      "-I${pkgs.libogg.dev}/include"
    ];

    cmakeFlags = [
      "-DOGG_LIBRARY=${libogg}/lib"
    ];
}
