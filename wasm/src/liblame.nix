{
  pkgs,
  pkgsWasm,
  stdenvWasm,
}: let
  lib = pkgs.lib;
in
  stdenvWasm.mkDerivation rec {
    name = "liblame";
    src = pkgs.lame.src;
    nativeBuildInputs = [
      pkgs.pkg-config
    ];

    NIX_CFLAGS_COMPILE = [
      "-D_WASI_EMULATED_SIGNAL=1"
      "-D_WASI_EMULATED_PROCESS_CLOCKS=1"
    ];

    NIX_LDFLAGS = [
      "-lwasi-emulated-signal"
      "-lwasi-emulated-process-clocks"
    ];
  }
