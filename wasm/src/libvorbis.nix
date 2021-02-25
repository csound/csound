{ pkgs ? import <nixpkgs> { }, static ? false }:

let lib = pkgs.lib;
    wasi-sdk-dyn = pkgs.callPackage ./wasi-sdk.nix { };
    wasi-sdk-static = pkgs.callPackage ./wasi-sdk-static.nix { };
    wasi-sdk = if static then wasi-sdk-static else wasi-sdk-dyn;

in pkgs.stdenvNoCC.mkDerivation rec {
    name = "libvorbis";
    src = pkgs.libvorbis.src;
    phases = [ "buildPhase" "installPhase" ];

    buildPhase = ''
      tar -xf $src --strip 1
      rm lib/psytune.c
      rm lib/barkmel.c
      rm lib/tone.c
      ${wasi-sdk}/bin/clang \
         -ffast-math -fsigned-char \
         --sysroot=${wasi-sdk}/share/wasi-sysroot \
         ${lib.optionalString (static == false) "--target=wasm32-unknown-emscripten" } \
         ${lib.optionalString (static == false) "-fPIC" } \
         -I${pkgs.libvorbis.dev}/include \
         -I${pkgs.libogg.dev}/include \
         -I./include \
         -I./lib \
         -O3 \
         -D__wasi__=1 \
         -D__wasm32__=1 \
         -D__NO_MATH_INLINES=1 \
         -D_REENTRANT=1 \
         -c \
         ./lib/*.c
    '';

    installPhase = ''
      mkdir -p $out/lib
      ${wasi-sdk}/bin/llvm-ar crS $out/lib/libvorbis.a ./*.o
      ${wasi-sdk}/bin/llvm-ranlib -U $out/lib/libvorbis.a
    '';
}
