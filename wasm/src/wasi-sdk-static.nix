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
  PREFIX = "${placeholder "out"}";

  postPatch = ''
    rm -rf src/*
    cp -rf ${wasilibc} src/wasi-libc
    cp -rf ${llvm-project} src/llvm-project
    cp -rf ${config} src/config
    chmod -R +rw src/
    substituteInPlace Makefile \
      --replace 'DESTDIR=$(abspath build/install)' \
                'DESTDIR='
  '';

  buildInputs = [ cmake git perl ninja python3 ];
  installPhase = "true";
}
