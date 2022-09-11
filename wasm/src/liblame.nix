{ pkgs ? import <nixpkgs> { }, static ? false }:

let lib = pkgs.lib;
    wasi-sdk-dyn = pkgs.callPackage ./wasi-sdk.nix { };
    wasi-sdk-static = pkgs.callPackage ./wasi-sdk-static.nix { };
    wasi-sdk = if static then wasi-sdk-static else wasi-sdk-dyn;

in pkgs.stdenvNoCC.mkDerivation rec {
    name = "liblame";
    src = pkgs.lame.src;
    phases = [ "buildPhase" "installPhase" ];

    buildPhase = ''
      tar -xf $src --strip 1
      ${wasi-sdk}/bin/clang \
         --sysroot=${wasi-sdk}/share/wasi-sysroot \
         ${lib.optionalString (static == false) "--target=wasm32-unknown-emscripten" } \
         ${lib.optionalString (static == false) "-fPIC" } \
        -O3 \
        -I include \
        -D__wasi__=1 \
        -D__wasm32__=1 \
        -Dieee754_float32_t=float \
        -Dieee754_float64_t=double \
        -c \
        ./libmp3lame/*.c
    '';

    installPhase = ''
      mkdir -p $out/lib
      ${wasi-sdk}/bin/llvm-ar crS $out/lib/liblame.a ./*.o
      ${wasi-sdk}/bin/llvm-ranlib -U $out/lib/liblame.a
    '';
}
