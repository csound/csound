{ pkgs, pkgsWasm, stdenvWasm }:

let lib = pkgs.lib;
    libogg = pkgs.callPackage ./libogg.nix { inherit pkgs pkgsWasm stdenvWasm; };

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
      "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
      "-DOGG_LIBRARY=${libogg}/lib"
    ];
}
