{
  pkgs ? import <nixpkgs> {},
  pkgsWasm ? pkgs.pkgsCross.wasi32,
  stdenvWasm ? pkgsWasm.clangStdenv,
}:

let
  csound-wasm = pkgs.callPackage ./csound.nix {inherit pkgs pkgsWasm stdenvWasm;};

in stdenvWasm.mkDerivation {
  name = "csound-wasm-plugin-example";
  buildInputs = [ csound-wasm ];
  unpackPhase = "true";
  dontStrip = true;

  buildPhase = ''
    cp ${./plugin_example.c} ./plugin_example.c

    echo "Compile plugin_example.wasm"
    $CC \
      -fPIC -fno-exceptions -fno-rtti \
      -mllvm -wasm-enable-sjlj \
      -D__wasi__=1 \
      -D__wasm32__=1 \
      -D_WASI_EMULATED_SIGNAL \
      -D_WASI_EMULATED_MMAN \
      -DUSE_DOUBLE=1 \
      -I${csound-wasm}/include \
      -I${csound-wasm}/include/csound \
      -c  plugin_example.c \
      -o plugin_example.o

    echo "Link together plugin_example.wasm"
    $CC \
      -Wl,-z,stack-size=128 \
      -Wl,--import-table \
      -Wl,--import-memory \
      -Wl,--export-all \
      -Wl,--no-entry \
      -lwasi-emulated-signal -lwasi-emulated-mman \
      plugin_example.o -o plugin_example.wasm
  '';

  installPhase = ''
    mkdir -p $out/lib
    cp ./plugin_example.wasm $out/lib
  '';
}
