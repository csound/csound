{
  system ? builtins.currentSystem,
  pkgs ? import <nixpkgs> {inherit system;},
  wasiToolchain ? import ./wasi-preview2-toolchain.nix {inherit system pkgs;},
  pkgsWasm ? wasiToolchain.pkgsWasm,
  # Match the browser binary selected from this package at build time.
  sharedMemory ? builtins.pathExists ../lib/csound-browser-realtime.wasm.z,
  testVariant ? "",
  stdenvWasm ? if sharedMemory
    then (import ./wasi-realtime-toolchain.nix { inherit pkgs pkgsWasm; }).stdenv
    else wasiToolchain.stdenv,
  csoundSdkArchive ? if sharedMemory
    then ../lib/csound-plugin-sdk.tar.gz
    else ../lib/csound-plugin-sdk-compat.tar.gz,
  useSdkArchive ? true,
}:

let
  maxMemory = builtins.fromJSON (builtins.readFile ./shared-memory-max.txt);
  csound-sdk = pkgs.callPackage ./csound_plugin_sdk.nix {
    inherit pkgs pkgsWasm stdenvWasm csoundSdkArchive useSdkArchive sharedMemory;
  };

in assert builtins.elem testVariant [ "" "missing_tls" "trap_ctor" ];
stdenvWasm.mkDerivation {
  name = "csound-wasm-plugin-example";
  buildInputs = [ csound-sdk ];
  unpackPhase = "true";
  dontStrip = true;

  buildPhase = ''
    cp ${./plugin_example.c} ./plugin_example.c

    echo "Compile plugin_example.wasm"
    $CC \
      -fPIC -fno-exceptions -fno-rtti -fno-builtin \
      -mllvm -wasm-enable-sjlj \
      ${pkgs.lib.optionalString sharedMemory "-mllvm -wasm-use-legacy-eh=false -DCSOUND_WASI_SHARED_PLUGIN=1"} \
      ${pkgs.lib.optionalString (testVariant == "trap_ctor") "-DCSOUND_WASI_PLUGIN_TRAP_CTOR=1"} \
      -D__wasi__=1 \
      -D__wasm32__=1 \
      -D_WASI_EMULATED_SIGNAL \
      -D_WASI_EMULATED_MMAN \
      -I${csound-sdk}/include \
      -I${csound-sdk}/include/csound \
      -c  plugin_example.c \
      -o plugin_example.o

    ${pkgs.lib.optionalString (!sharedMemory) ''
    # scripts/nixpkgs-pin.sh pins nixpkgs d3b13ad75cc492690728f9307e443dec9ef9a20e
    # (wasi-libc SDK 32, Clang 21). This source path and its C_LONGJMP tag ABI
    # are version-sensitive; recheck both when changing that pin. SDK 32's
    # rt.c uses __builtin_wasm_throw(1, ...) without a Clang-version branch.
    # Compile it as PIC. The loader supplies a private exception tag, and the
    # plugin owns its jump buffers.
    $CC -fPIC -mllvm -wasm-enable-sjlj \
      -c ${pkgsWasm.wasilibc.src}/libc-top-half/musl/src/setjmp/wasm32/rt.c \
      -o setjmp.o

    ''}

    echo "Link together plugin_example.wasm"
    $CC \
      -shared \
      -nostdlib \
      -nostartfiles \
      -Wl,--experimental-pic \
      -Wl,--no-entry \
      -Wl,--import-table \
      -Wl,--import-memory \
      ${pkgs.lib.optionalString sharedMemory "-Wl,--shared-memory -Wl,--max-memory=${toString maxMemory}"} \
      ${pkgs.lib.optionalString (sharedMemory && testVariant != "missing_tls") "-Wl,--export=__wasm_init_tls -Wl,--export=__tls_size -Wl,--export=__tls_align"} \
      -Wl,--allow-undefined \
      -Wl,--export=__wasm_call_ctors \
      -Wl,--export-if-defined=__wasm_apply_data_relocs \
      -Wl,--export=csound_opcode_init \
      -Wl,--export=csoundModuleInfo \
      -Wl,--export=plugin_local_data \
      plugin_example.o ${pkgs.lib.optionalString (!sharedMemory) "setjmp.o"} -o plugin_example.wasm
  '';

  installPhase = ''
    mkdir -p $out/lib
    cp ./plugin_example.wasm $out/lib/plugin_example${pkgs.lib.optionalString sharedMemory "_shared"}${pkgs.lib.optionalString (testVariant != "") "_${testVariant}"}.wasm
  '';
}
