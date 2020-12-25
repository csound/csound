{ pkgs ? import <nixpkgs> { } }:

let
  wasi-sdk = pkgs.callPackage ./wasi-sdk.nix { };
  wasilibc-fpic = pkgs.callPackage ./wasilibc.fpic.nix { };
  csound = pkgs.callPackage ./csound.nix {};
  libsndfile = pkgs.callPackage ./libsndfile.nix {};

in pkgs.stdenv.mkDerivation {
  name = "csound-wasm-plugin-example";
  buildInputs = [ csound ];
  unpackPhase = "true";

  buildPhase = ''
    echo "Compile plugin_example.wasm"
    ${wasi-sdk}/bin/clang \
      -nostdlib --target=wasm32-unknown-emscripten \
      --sysroot=${wasi-sdk}/share/wasi-sysroot \
      --target=wasm32-unknown-emscripten \
      -fno-exceptions -fPIC -O0 \
      -D__wasi__=1 \
      -D__wasm32__=1 \
      -I${csound}/include \
      -include ${csound}/include/H/prototyp.h \
      -c ${./plugin_example.c}

    echo "Link togeather plugin_example.wasm"
    ${wasi-sdk}/bin/wasm-ld --lto-O0 \
      --shared \
      --no-entry \
      -error-limit=0 \
       --export=init \
      -L${wasilibc-fpic}/lib -lc \
      -L${csound}/lib -lcsound \
      *.o -o plugin_example.wasm
  '';

  installPhase = ''
    mkdir -p $out/lib
    cp ./plugin_example.wasm $out/lib
  '';
}
