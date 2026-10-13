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
      -fPIC -fno-exceptions -fno-rtti \
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

    echo "Link together plugin_example.wasm"
    $CC \
      -shared \
      -nostdlib \
      -nostartfiles \
      -Wl,--experimental-pic \
      -Wl,--no-entry \
      -Wl,--import-table \
      -Wl,--import-memory \
      -Wl,--import-undefined \
      -Wl,--export=__wasm_call_ctors \
      -Wl,--export=csound_opcode_init \
      -Wl,--export=csoundModuleInfo \
      plugin_example.o -o plugin_example.wasm
  '';

  installPhase = ''
    mkdir -p $out/lib
    cp ./plugin_example.wasm $out/lib
  '';
}
