{ stdenv, fetchFromGitHub, fetchgit, fetchurl, lib, cmake, git, perl, ninja, python3 }:

let wasilibc = fetchFromGitHub {
  owner = "WebAssembly";
  repo = "wasi-libc";
  rev = "ad5133410f66b93a2381db5b542aad5e0964db96";
  sha256 = "sha256-gw6flqJv4x//V3FdqDx6yXhYVQjJ2S2xx0tZShFjmsQ=";
};

llvm-project = fetchFromGitHub {
  owner = "llvm";
  repo = "llvm-project";
  rev = "309f1e4ac8cca1ba1f0e28eeae8e2926dc387d04";
  sha256 = "WwDcWRf7Gu7b2a17mHbke31xSHDYA7CHEi28gR+Zd90=";
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
    rev = "77ba98a998cb9f2a63ab3a5f94bbabd069f65ff0";
    sha256 = "sha256-IG0kxt6geu1Y7qHWSWjp0LrPevU8eC+kCS/yZD/j5YE=";
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
      --replace '<xlocale.h>' '<__support/musl/xlocale.h>'
    substituteInPlace src/llvm-project/libcxxabi/src/stdlib_new_delete.cpp \
      --replace '__EMSCRIPTEN__' '__wasi__' || exit 1
    substituteInPlace src/llvm-project/libcxx/src/new.cpp \
      --replace '__EMSCRIPTEN__' '__wasi__' || exit 1
  '';

  buildInputs = [ cmake git perl ninja python3 ];
  installPhase = "true";
}
