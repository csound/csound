{ pkgs ? import <nixpkgs> { } }:

let
  wasi-sdk = pkgs.callPackage ./wasi-sdk.nix { };
  csound-wasm = pkgs.callPackage ./csound.nix {};

in pkgs.stdenv.mkDerivation {
  name = "csound-wasm-plugin-example";
  buildInputs = [ csound-wasm ];
  unpackPhase = "true";
  dontStrip = true;

  buildPhase = ''

    cp ${./plugin_example.c} ./plugin_example.c

    echo "Compile plugin_example.wasm"
    ${wasi-sdk}/bin/clang \
       -fPIC -fno-exceptions -fno-rtti \
      --target=wasm32-unknown-emscripten \
      --sysroot=${wasi-sdk}/share/wasi-sysroot \
      -D__wasi__=1 \
      -D__wasm32__=1 \
      -D_WASI_EMULATED_SIGNAL \
      -D_WASI_EMULATED_MMAN \
      -DUSE_DOUBLE=1 \
      -I${csound-wasm}/include \
      -c  plugin_example.c \
      -o plugin_example.o

    echo "Link togeather plugin_example.wasm"
    ${wasi-sdk}/bin/wasm-ld \
      --shared \
      --import-table \
      --import-memory \
      --export-all \
      --no-entry \
      -L${wasi-sdk}/share/wasi-sysroot/lib/wasm32-unknown-emscripten \
      -L${csound-wasm}/lib -lcsound-wasm \
      -lc -lwasi-emulated-signal -lwasi-emulated-mman \
      plugin_example.o -o plugin_example.wasm
  '';

  installPhase = ''
    mkdir -p $out/lib
    cp ./plugin_example.wasm $out/lib
  '';
}
