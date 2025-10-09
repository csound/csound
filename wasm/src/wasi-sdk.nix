{
  cargo,
  stdenvNoCC,
  fetchFromGitHub,
  fetchgit,
  fetchurl,
  lib,
  cmake,
  git,
  perl,
  ninja,
  python3,
  overrideCC,
  llvmPackages_latest,
  wrapCCWith
}:

let
  wasilibc = fetchFromGitHub {
    owner = "WebAssembly";
    repo = "wasi-libc";
    rev = "008d705c9d16dc9057bb6efc220fb539f1fb7fb5";
    hash = "sha256-6ZKoruvxl50/aXuDGo4P0H12VnXFy/Pg8oGOHPXxrbI=";
  };

  llvm-project = fetchFromGitHub {
    owner = "llvm";
    repo = "llvm-project";
    rev = "d7eade1379606b984026ec06ea8d8eaa8a6e10ce";
    hash = "sha256-cUT2WH4C0n+emc4s0Dh3w8AUzUq5DIS5JKXkLy4Mr1g=";
  };

  config = fetchgit {
    url = "https://git.savannah.gnu.org/git/config.git";
    rev = "2593751ef276497e312d7c4ce7fd049614c7bf80";
    sha256 = "1sh410ncfs9fwxw03m1r4lcm10iv305g0jb2bb2yvgzlpb28lsz9";
  };

  llvm = llvmPackages_latest;
  filteredClang = wrapCCWith {
    cc = llvm.clang;
    extraBuildCommands = ''
      for f in $out/nix-support/cc-cflags $out/nix-support/libc-cflags; do
        if [ -f "$f" ]; then
          substituteInPlace "$f" --replace-fail "-fzero-call-used-regs=used-gpr" ""
        fi
      done
    '';
  };

  # Make a stdenv that uses our filtered Clang
  clangStdenvFiltered = overrideCC llvm.stdenv filteredClang;

in
stdenvNoCC.mkDerivation {
  name = "wasi-sdk-0.0.0";
  src = fetchFromGitHub {
    owner = "WebAssembly";
    repo = "wasi-sdk";
    rev = "d90d7de10e2208905b1cdf29817f89c3ed68cbcd";
    sha256 = "sha256-uXyk5ixJEPR3yaAJXzlIrxEdMWfYXmVwmn1wexe08Mc=";
    fetchSubmodules = false;
  };

  hardeningDisable = [ "zerocallusedregs" ];

  dontUseCmakeConfigure = true;
  dontUseNinjaBuild = true;
  dontUseNinjaInstall = true;
  dontStrip = true;
  PREFIX = "${placeholder "out"}";
  WASI_SDK_VERSION = "27";
  GIT_COMMIT = "000000000000";
  GIT_COMMIT_SRC_WASI_LIBC = "aaaaaaaaaaaa";
  GIT_COMMIT_SRC_LLVM_PROJECT = "bbbbbbbbbbbb";
  GIT_COMMIT_SRC_CONFIG = "cccccccccccc";

  postPatch = ''
    rm -rf src/*
    cp -rf ${wasilibc} src/wasi-libc
    cp -rf ${llvm-project} src/llvm-project
    cp -rf ${config} src/config
    chmod -R +rw src/

  patchShebangs version.py

  substituteInPlace version.py \
    --replace-fail "def git_version():" \
'def git_version():
    v = os.environ.get("WASI_SDK_VERSION")
    if v:
        return v'

  substituteInPlace version.py \
    --replace-fail "def git_commit(dir):" \
'def git_commit(dir):
    k = "GIT_COMMIT_" + dir.replace("/", "_").upper()
    v = os.environ.get(k) or os.environ.get("GIT_COMMIT")
    if v:
        return v[:GIT_REF_LEN]'

  # we don't need rust support
  substituteInPlace cmake/wasi-sdk-toolchain.cmake \
    --replace-fail "cargo install" "echo cargo install"
  '';

  buildPhase = ''
    cmake -G Ninja -B build/toolchain -S . -DWASI_SDK_BUILD_TOOLCHAIN=ON -DCMAKE_INSTALL_PREFIX=build/install
    cmake --build build/toolchain --target install
  '';

  buildInputs = [
    cmake
    llvmPackages_latest.clang
    git
    perl
    ninja
    python3
  ];
  installPhase = "true";
}
