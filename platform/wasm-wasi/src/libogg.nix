{
  pkgs,
  pkgsWasm,
  stdenvWasm,
}: let
  lib = pkgs.lib;
in
  stdenvWasm.mkDerivation rec {
    name = "libogg";
    src = pkgs.libogg.src;
    nativeBuildInputs = [
      pkgs.cmake
      pkgs.pkg-config
    ];
  }
