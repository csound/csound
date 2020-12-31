{ stdenv, fetchFromGitHub, fetchgit, fetchurl, lib, cmake, git, perl, ninja, python3 }:

let wasilibc = fetchFromGitHub {
  owner = "WebAssembly";
  repo = "wasi-libc";
  rev = "378fd4b21aab6d390f3a1c1817d53c422ad00a62";
  sha256 = "0h5g0q5j9cni7jab0b6bzkw5xm1b1am0dws2skq3cc9c9rnbn1ga";
};

llvm-project = fetchFromGitHub {
  owner = "llvm";
  repo = "llvm-project";
  rev = "9a6de74d5a9e11a7865ce4873ff3297b7efbb673";
  sha256 = "1xcr16xk30a4zjz8fpqacqcfarl2dpv6jy1vnhqi1yl5i70zx6s4";
};

config = fetchgit {
  url = "https://git.savannah.gnu.org/git/config.git";
  rev = "2593751ef276497e312d7c4ce7fd049614c7bf80";
  sha256 = "1sh410ncfs9fwxw03m1r4lcm10iv305g0jb2bb2yvgzlpb28lsz9";
};

emscripten_new_cpp_patch = fetchurl {
  url = "https://raw.githubusercontent.com/emscripten-core/emscripten/a153b417d34cb6f872310f5969e522d255d7294a/system/lib/libcxx/new.cpp";
  sha256 = "0ghdfdjx71gxsxhkl6y7ri9x841h32y140m9n2qjhd27p013sphb";
};

emscripten_new_delete_cppabi_patch = fetchurl {
  url = "https://raw.githubusercontent.com/emscripten-core/emscripten/a153b417d34cb6f872310f5969e522d255d7294a/system/lib/libcxxabi/src/stdlib_new_delete.cpp";
  sha256 = "099d5458xcbrcrhv3l1643ms529753q1yrr6m349rcwd33ad6g9r";
};

in stdenv.mkDerivation {
  name = "wasi-sdk-0.0.0";
  src = fetchFromGitHub {
    owner = "WebAssembly";
    repo = "wasi-sdk";
    rev = "b36c433738f0c29160a5ac1c1cee1b1b884bf4a0";
    sha256 = "0dn0y1rzcmbzmymy5z73x234vwhg0qcjmw0yvhankc27z355f7ss";
    fetchSubmodules = false;
  };

  dontUseCmakeConfigure = true;
  dontUseNinjaBuild = true;
  dontUseNinjaInstall = true;
  dontStrip = true;
  WASM_CFLAGS = "-fPIC -D__wasi__=1 -D__wasm32__=1";
  PREFIX = "${placeholder "out"}";

  postPatch = ''
    rm -rf src/*
    cp -rf ${wasilibc} src/wasi-libc
    cp -rf ${llvm-project} src/llvm-project
    cp -rf ${config} src/config
    chmod -R +rw src/
    sed -i -e 's/diff -wur.*//g' src/wasi-libc/Makefile
    cp ${emscripten_new_cpp_patch} src/llvm-project/libcxx/src/new.cpp
    cp ${emscripten_new_delete_cppabi_patch} src/llvm-project/libcxxabi/src/stdlib_new_delete.cpp

    substituteInPlace src/wasi-libc/Makefile \
      --replace 'wasm32-wasi' 'wasm32-unknown-emscripten'

    substituteInPlace Makefile \
      --replace 'DESTDIR=$(abspath build/install)' \
                'DESTDIR=' \
      --replace 'COMPILER_RT_HAS_FPIC_FLAG=OFF' \
                'COMPILER_RT_HAS_FPIC_FLAG=ON' \
      --replace 'DLIBCXXABI_ENABLE_PIC:BOOL=OFF' \
                'DLIBCXXABI_ENABLE_PIC:BOOL=ON' \
      --replace 'wasi-sysroot"' \
                'wasi-sysroot -fPIC -fno-exceptions -D__wasi__=1 -D__wasm32__=1 -D_LIBCXXABI_NO_EXCEPTIONS=1"' \
      --replace 'wasm32-wasi' \
                'wasm32-unknown-emscripten'

    substituteInPlace wasi-sdk.cmake \
      --replace 'wasm32-wasi' 'wasm32-unknown-emscripten'
    substituteInPlace src/llvm-project/libcxx/include/__locale \
      --replace '<xlocale.h>' '<support/musl/xlocale.h>'
    substituteInPlace src/llvm-project/libcxxabi/src/stdlib_new_delete.cpp \
      --replace '__EMSCRIPTEN__' '__wasi__' || exit 1
    substituteInPlace src/llvm-project/libcxx/src/new.cpp \
      --replace '__EMSCRIPTEN__' '__wasi__' || exit 1
  '';

  buildInputs = [ cmake git perl ninja python3 ];
  installPhase = "true";
}
