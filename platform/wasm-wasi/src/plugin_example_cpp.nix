{
  system ? builtins.currentSystem,
  pkgs ? import <nixpkgs> {inherit system;},
  pkgsWasm ? pkgs.pkgsCross.wasi32,
  # Match the browser binary selected from this package at build time.
  sharedMemory ? builtins.pathExists ../lib/csound-browser-realtime.wasm.z,
  stdenvWasm ? if sharedMemory
    then (import ./wasi-realtime-toolchain.nix { inherit pkgs pkgsWasm; }).stdenv
    else pkgsWasm.clangStdenv,
  csoundSdkArchive ? if sharedMemory
    then ../lib/csound-plugin-sdk.tar.gz
    else ../lib/csound-plugin-sdk-compat.tar.gz,
  useSdkArchive ? true,
}: let
  maxMemory = builtins.fromJSON (builtins.readFile ./shared-memory-max.txt);
  csound-sdk = pkgs.callPackage ./csound_plugin_sdk.nix {
    inherit pkgs pkgsWasm stdenvWasm csoundSdkArchive useSdkArchive sharedMemory;
  };
in
  stdenvWasm.mkDerivation {
    name = "csound-wasm-cpp-plugin-example";

    buildInputs = [csound-sdk];
    unpackPhase = "true";

    dontStrip = true;

    buildPhase = ''
      cp ${./plugin_example_cpp.cpp} ./plugin_example_cpp.cpp

      echo "Compile plugin_example_cpp.wasm"
      $CXX \
        -fPIC -fno-exceptions -fno-rtti \
        -mllvm -wasm-enable-sjlj \
        ${pkgs.lib.optionalString sharedMemory "-mllvm -wasm-use-legacy-eh=false -DCSOUND_WASI_SHARED_PLUGIN=1"} \
        -D__wasi__=1 \
        -D__wasm32__=1 \
        -D_WASI_EMULATED_SIGNAL \
        -D_WASI_EMULATED_MMAN \
        -DUSE_DOUBLE=1 \
        -I${csound-sdk}/include \
        -I${csound-sdk}/include/csound \
        -c plugin_example_cpp.cpp \
        -o plugin_example_cpp.o

      echo "Link together plugin_example_cpp.wasm"
      $CXX \
        -shared \
        -nostdlib \
        -nostartfiles \
        -Wl,--experimental-pic \
        -Wl,--no-entry \
        -Wl,--import-table \
        -Wl,--import-memory \
        ${pkgs.lib.optionalString sharedMemory "-Wl,--shared-memory -Wl,--max-memory=${toString maxMemory} -Wl,--export=__wasm_init_tls -Wl,--export=__tls_size -Wl,--export=__tls_align"} \
        -Wl,--import-undefined \
        -Wl,--export=__wasm_call_ctors \
        -Wl,--export-if-defined=__wasm_apply_data_relocs \
        -Wl,--export=csoundModuleInit \
        -Wl,--export=csoundModuleInfo \
        -Wl,--export-if-defined=csoundModuleCreate \
        -Wl,--export-if-defined=csoundModuleDestroy \
        plugin_example_cpp.o -o plugin_example_cpp.wasm
    '';

    installPhase = ''
      mkdir -p $out/lib
      cp ./plugin_example_cpp.wasm $out/lib/plugin_example_cpp${pkgs.lib.optionalString sharedMemory "_shared"}.wasm
    '';
  }
