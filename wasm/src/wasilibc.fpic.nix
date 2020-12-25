{ pkgs, stdenv, fetchFromGitHub, lib }:

let
  wasi-sdk = pkgs.callPackage ./wasi-sdk.nix { };
in stdenv.mkDerivation {
  name = "wasilibc";
  src = fetchFromGitHub {
    owner = "WebAssembly";
    repo = "wasi-libc";
    rev = "378fd4b21aab6d390f3a1c1817d53c422ad00a62";
    sha256 = "0h5g0q5j9cni7jab0b6bzkw5xm1b1am0dws2skq3cc9c9rnbn1ga";
  };
  WASM_CFLAGS = "-fPIC -D__wasi__=1 -D__wasm32__=1";
  dontStrip = true;
  makeFlags = [
    "WASM_CC=${wasi-sdk}/bin/clang"
    "WASM_NM=${wasi-sdk}/bin/nm"
    "WASM_AR=${wasi-sdk}/bin/ar"
    "WASM_LD=${wasi-sdk}/bin/wasm-ld"
    "INSTALL_DIR=${placeholder "out"}"
  ];

  postPatch = ''
    sed -i -e 's/diff -wur.*//g' Makefile
    substituteInPlace Makefile \
      --replace 'wasm32-wasi' 'wasm32-unknown-emscripten'
  '';

  postInstall = ''
    mv $out/lib/*/* $out/lib
    ln -s $out/share/wasm32-wasi/undefined-symbols.txt $out/lib/wasi.imports
  '';
}
