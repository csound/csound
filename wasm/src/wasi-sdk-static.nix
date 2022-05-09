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
  PREFIX = "${placeholder "out"}";

  postPatch = ''
    rm -rf src/*
    cp -rf ${wasilibc} src/wasi-libc
    cp -rf ${llvm-project} src/llvm-project
    cp -rf ${config} src/config
    chmod -R +rw src/
    substituteInPlace Makefile \
      --replace 'DESTDIR=$(abspath build/install)' \
                'DESTDIR=' \
      --replace 'install-clang ' 'install-clang install-llc install-opt '
  '';

  buildInputs = [ cmake git perl ninja python3 ];
  installPhase = "true";
}
