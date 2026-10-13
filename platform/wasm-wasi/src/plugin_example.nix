{
  system ? builtins.currentSystem,
  pkgs ? import <nixpkgs> {inherit system;},
  pkgsWasm ? pkgs.pkgsCross.wasi32,
  stdenvWasm ? pkgsWasm.clangStdenv,
  csoundSdkArchive ? ../lib/csound-plugin-sdk.tar.gz,
  useSdkArchive ? true,
}:

let
  csound-sdk = pkgs.callPackage ./csound_plugin_sdk.nix {
    inherit pkgs pkgsWasm stdenvWasm csoundSdkArchive useSdkArchive;
  };

in stdenvWasm.mkDerivation {
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
      -D__wasi__=1 \
      -D__wasm32__=1 \
      -D_WASI_EMULATED_SIGNAL \
      -D_WASI_EMULATED_MMAN \
      -DUSE_DOUBLE=1 \
      -I${csound-sdk}/include \
      -I${csound-sdk}/include/csound \
      -c  plugin_example.c \
      -o plugin_example.o

    # scripts/nixpkgs-pin.sh pins nixpkgs 4db2c220f32fd162658ed1b7bb2f46a82996ddbe
    # (wasi-libc SDK 27, Clang 21). This source path and its C_LONGJMP tag ABI
    # are version-sensitive; recheck both when changing that pin. SDK 27's
    # rt.c uses __builtin_wasm_throw(1, ...) without a Clang-version branch.
    # Compile it as PIC. The loader supplies a private exception tag, and the
    # plugin owns its jump buffers.
    $CC -fPIC -mllvm -wasm-enable-sjlj \
      -c ${pkgsWasm.wasilibc.src}/libc-top-half/musl/src/setjmp/wasm32/rt.c \
      -o setjmp.o

    echo "Link together plugin_example.wasm"
    $CC \
      -shared \
      -nostdlib \
      -nostartfiles \
      -Wl,--experimental-pic \
      -Wl,--no-entry \
      -Wl,--import-table \
      -Wl,--import-memory \
      -Wl,--allow-undefined \
      -Wl,--export=__wasm_call_ctors \
      -Wl,--export-if-defined=__wasm_apply_data_relocs \
      -Wl,--export=csound_opcode_init \
      -Wl,--export=csoundModuleInfo \
      -Wl,--export=plugin_local_data \
      plugin_example.o setjmp.o -o plugin_example.wasm
  '';

  installPhase = ''
    mkdir -p $out/lib
    cp ./plugin_example.wasm $out/lib
  '';
}
