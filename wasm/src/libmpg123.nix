{ pkgs ? import <nixpkgs> { }, static ? false }:

let lib = pkgs.lib;
    wasi-sdk-dyn = pkgs.callPackage ./wasi-sdk.nix { };
    wasi-sdk-static = pkgs.callPackage ./wasi-sdk-static.nix { };
    wasi-sdk = if static then wasi-sdk-static else wasi-sdk-dyn;


in pkgs.stdenvNoCC.mkDerivation rec {
  name = "mpg123";
  version = "1.29.3";

  src = pkgs.fetchurl {
    url = "mirror://sourceforge/${name}/${name}-${version}.tar.bz2";
    sha256 = "sha256-ljiF2Mx3Ji8ot3GHx9GJ4yGV5kJE3iUwt5jd8yGD6Ec=";
  };

  phases = [ "unpackPhase" "buildPhase" "installPhase" ];

  buildPhase = ''
    mkdir -p include
    cp ${./libmpg123.config.in} include/config.h
    mv src/libmpg123/mpg123.h.in src/libmpg123/mpg123.h
    rm src/libmpg123/calctables.c
    rm src/libmpg123/getcpuflags_arm.c
    rm src/libmpg123/dct64_altivec.c
    rm src/libmpg123/synth_altivec.c
    rm src/libmpg123/synth_i486.c
    rm src/libmpg123/testcpu.c

    substituteInPlace src/libmpg123/mpg123.h \
      --replace '@API_VERSION@' '"${version}"' \
      --replace '@BUILD_NO_LARGENAME@' "0" \
      --replace '@INCLUDE_STDLIB_H@' "" \
      --replace '@INCLUDE_SYS_TYPE_H@' ""


    # in wasi-libc lseek is a macro, which causes an annoying issue
    # where lseek is used as a symbol in struct
    substituteInPlace src/libmpg123/reader.h \
      --replace 'off_t   (*lseek)' 'off_t   (*lseek_fix)'

    substituteInPlace src/libmpg123/readers.c \
      --replace 'rdat->lseek' 'rdat->lseek_fix' \
      --replace 'rdat.lseek' 'rdat.lseek_fix'

    ${wasi-sdk}/bin/clang \
       --sysroot=${wasi-sdk}/share/wasi-sysroot \
       ${lib.optionalString (static == false) "--target=wasm32-unknown-emscripten" } \
       ${lib.optionalString (static == false) "-fPIC" } \
      -O3 \
      -I include \
      -I src \
      -I src/compat \
      -I src/libmpg123 \
      -D__wasi__=1 \
      -D__wasm32__=1 \
      -Dlfs_alias_type=double \
      -Dlfs_alias_t=double \
      -DLFS_ALIAS_BITS=8 \
      -c \
      ./src/compat/*.c \
      ./src/libmpg123/*.c
  '';

  installPhase = ''
    mkdir -p $out/lib
    ${wasi-sdk}/bin/llvm-ar crS $out/lib/libmpg123.a ./*.o
    ${wasi-sdk}/bin/llvm-ranlib -U $out/lib/libmpg123.a
  '';
}
