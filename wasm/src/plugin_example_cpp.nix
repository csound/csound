{
  pkgs ? import <nixpkgs> {},
  pkgsWasm ? pkgs.pkgsCross.wasi32,
  stdenvWasm ? pkgsWasm.clangStdenv,
}: let
  csound-wasm = pkgs.callPackage ./csound.nix {inherit pkgs pkgsWasm stdenvWasm;};
in
  stdenvWasm.mkDerivation {
    name = "csound-wasm-cpp-plugin-example";

    buildInputs = [csound-wasm];
    unpackPhase = "true";

    dontStrip = true;

    buildPhase = ''
      cp ${./plugin_example_cpp.cpp} ./plugin_example_cpp.cpp

      echo "Compile plugin_example_cpp.wasm"
      $CXX \
        -fPIC -fno-exceptions -fno-rtti \
        -mllvm -wasm-enable-sjlj \
        -D__wasi__=1 \
        -D__wasm32__=1 \
        -D_WASI_EMULATED_SIGNAL \
        -D_WASI_EMULATED_MMAN \
        -DUSE_DOUBLE=1 \
        -I${csound-wasm}/include \
        -I${csound-wasm}/include/csound \
        -c plugin_example_cpp.cpp \
        -o plugin_example_cpp.o

      echo "Link together plugin_example_cpp.wasm"
      $CXX \
        -Wl,-z,stack-size=128 \
        -Wl,--no-entry \
        -Wl,--import-table \
        -Wl,--import-memory \
        -Wl,--export=__wasm_call_ctors \
        -Wl,--export=csoundModuleInit \
        -Wl,--export-if-defined=csoundModuleCreate \
        -Wl,--export-if-defined=csoundModuleDestroy \
        -lwasi-emulated-getpid -lwasi-emulated-signal -lwasi-emulated-mman -lwasi-emulated-process-clocks \
        plugin_example_cpp.o -o plugin_example_cpp.wasm
    '';

    installPhase = ''
      mkdir -p $out/lib
      cp ./plugin_example_cpp.wasm $out/lib
    '';
  }
