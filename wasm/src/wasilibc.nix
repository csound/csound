{ stdenv, fetchFromGitHub, lib }:

stdenv.mkDerivation {
  name = "wasilibc";
  src = fetchFromGitHub {
    owner = "WebAssembly";
    repo = "wasi-libc";
    rev = "00cc5944dfc8c85ab5c5bee4cdef221afa2121f7";
    sha256 = "1i41lmgpdp00pn5r5ddd2hzmk0dv0l2pzbc4b84nrsiwp605m10r";
  };
  makeFlags = [
    "WASM_CC=${stdenv.cc.targetPrefix}cc"
    "WASM_NM=${stdenv.cc.targetPrefix}nm"
    "WASM_AR=${stdenv.cc.targetPrefix}ar"
    "INSTALL_DIR=${placeholder "out"}"
  ];
  postInstall = ''
    mv $out/lib/*/* $out/lib
    ln -s $out/share/wasm32-wasi/undefined-symbols.txt $out/lib/wasi.imports
  '';

  meta = {
    description = "WASI libc implementation for WebAssembly";
    homepage = "https://wasi.dev";
    platforms = lib.platforms.wasi;
    maintainers = [ lib.maintainers.matthewbauer ];
  };
}
