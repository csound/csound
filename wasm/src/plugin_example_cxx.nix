## To see the source code used in this example
## visit https://github.com/ketchupok/half-physler

{ pkgs ? import <nixpkgs> { } }:

let
  wasi-sdk = pkgs.callPackage ./wasi-sdk.nix { };
  csound-wasm = pkgs.callPackage ./csound.nix {};

in pkgs.stdenv.mkDerivation {

  name = "csound-wasm-cxx-plugin-example";

  buildInputs = [ csound-wasm ];

  src = pkgs.fetchFromGitHub {
    owner = "ketchupok";
    repo = "half-physler";
    rev = "3dc1dfa9f40a2246c96babeee242e2458f56a6ac";
    sha256 = "0k9av2aw64jc4l1hg08jvc1gn57knqp92ca29k2ra5mk2mwhm78q";
  };

  dontStrip = true;

  # wasm currently doesn't support atomics
  # here we just remove the import statement.
  patchPhase = ''
    find ./ -type f -exec sed -i -e 's/#include <atomic>//g' {} \;
  '';

  buildPhase = ''
    echo "Compile plugin_example_cxx.wasm"
    ${wasi-sdk}/bin/clang  \
      -fPIC -fno-exceptions -fno-rtti \
      --target=wasm32-unknown-emscripten \
      --sysroot=${wasi-sdk}/share/wasi-sysroot \
      -D__wasi__=1 \
      -D__wasm32__=1 \
      -D_WASI_EMULATED_SIGNAL \
      -D_WASI_EMULATED_MMAN \
      -D__wasilibc_printscan_no_long_double \
      -D__wasilibc_printscan_no_floating_point \
      -DUSE_DOUBLE=1 \
      -I${csound-wasm}/include \
      -I./src \
      -c src/const.cpp \
      src/tube.cpp \
      src/opcode_registers.cpp \
      resonators/resontube.cpp

    echo "Link togeather plugin_example_cxx.wasm"
    ${wasi-sdk}/bin/wasm-ld --shared \
      --import-table --import-memory \
      --export-all \
      --no-entry \
      -L${csound-wasm}/lib \
      -L${wasi-sdk}/share/wasi-sysroot/lib/wasm32-unknown-emscripten \
      -lcsound-dylib -lc -lc++ -lc++abi -lwasi-emulated-signal -lwasi-emulated-mman \
      *.o -o plugin_example_cxx.wasm
  '';

  installPhase = ''
    mkdir -p $out/lib
    cp ./plugin_example_cxx.wasm $out/lib
  '';
}
