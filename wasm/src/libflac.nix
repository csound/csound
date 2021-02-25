{ pkgs ? import <nixpkgs> { }, static ? false }:

let lib = pkgs.lib;
    wasi-sdk-dyn = pkgs.callPackage ./wasi-sdk.nix { };
    wasi-sdk-static = pkgs.callPackage ./wasi-sdk-static.nix { };
    wasi-sdk = if static then wasi-sdk-static else wasi-sdk-dyn;

in pkgs.stdenvNoCC.mkDerivation rec {
    name = "libflac";
    src = pkgs.flac.src;
    phases = [ "unpackPhase" "patchPhase" "buildPhase" "installPhase" ];

    unpackPhase = "tar -xf $src --strip 1";

   patchPhase = ''
     rm ./src/libFLAC/windows_unicode_filenames.c
     mv ./include/share/win_utf8_io.h ./include/io.h
   '';

    buildPhase = ''
      ${wasi-sdk}/bin/clang \
         --sysroot=${wasi-sdk}/share/wasi-sysroot \
         ${lib.optionalString (static == false) "--target=wasm32-unknown-emscripten" } \
         ${lib.optionalString (static == false) "-fPIC" } \
         -I${pkgs.libogg.dev}/include \
         -I${pkgs.flac.dev}/include \
         -I./src/libFLAC/include \
         -I./include \
         -I${pkgs.libsndfile.dev}/include \
         -O2 \
         -D__wasi__=1 \
         -D__wasm32__=1 \
         -DHAVE_LROUND=1 \
         -DPACKAGE_VERSION='"1.3.3"' \
         -DFLAC__HAS_OGG=1 \
         -c \
         ./src/libFLAC/*.c
    '';

    installPhase = ''
      mkdir -p $out/lib
      ${wasi-sdk}/bin/llvm-ar crS $out/lib/libflac.a ./*.o
      ${wasi-sdk}/bin/llvm-ranlib -U $out/lib/libflac.a
    '';
}
