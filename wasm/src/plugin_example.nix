{ pkgs ? import <nixpkgs> { } }:

let
  wasi-sdk = pkgs.callPackage ./wasi-sdk.nix { };
  csound = pkgs.callPackage ./csound.nix {};

in pkgs.stdenv.mkDerivation {
  name = "csound-wasm-plugin-example";
  buildInputs = [ csound ];
  unpackPhase = "true";
  dontStrip = true;

  buildPhase = ''
    echo "Compile plugin_example.wasm"
    ${wasi-sdk}/bin/clang  \
      -fPIC -fno-exceptions -fno-rtti \
      --target=wasm32-unknown-emscripten \
      --sysroot=${wasi-sdk}/share/wasi-sysroot \
      -D__wasi__=1 \
      -D__wasm32__=1 \
      -D_WASI_EMULATED_SIGNAL \
      -D_WASI_EMULATED_MMAN \
      -I${csound}/include \
      -c ${./plugin_example.c}

    echo "Link togeather plugin_example.wasm"
    ${wasi-sdk}/bin/wasm-ld --shared \
      --import-table --import-memory \
      --export=__wasm_call_ctors \
      --export-dynamic --no-entry \
      -L${csound}/lib \
      -L${wasi-sdk}/share/wasi-sysroot/lib/wasm32-unknown-emscripten \
      -lcsound -lc -lc++ -lc++abi -lwasi-emulated-signal -lwasi-emulated-mman \
      *.o -o plugin_example.wasm
  '';

  installPhase = ''
    mkdir -p $out/lib
    cp ./plugin_example.wasm $out/lib
  '';
}
