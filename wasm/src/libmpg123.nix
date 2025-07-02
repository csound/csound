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
    rm src/libmpg123/lfs_alias.c
    rm src/libmpg123/lfs_wrap.c
    rm src/libmpg123/calctables.c
    rm src/libmpg123/getcpuflags_arm.c
    rm src/libmpg123/dct64_altivec.c
    rm src/libmpg123/dct64_i486.c
    rm src/libmpg123/synth_altivec.c
    rm src/libmpg123/synth_i486.c
    rm src/libmpg123/testcpu.c

    substituteInPlace src/libmpg123/mpg123.h \
      --replace '@API_VERSION@' '46' \
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
      -DOPT_GENERIC_DITHER=1 \
      -DOPT_GENERIC=1 \
      -DREAL_IS_FLOAT=1 \
      -DNO_ICY=1 \
      -DNO_NTOM=1 \
      -DNO_STRING=1 \
      -DNO_DOWNSAMPLE=1 \
      -DNO_ID3V2=1 \
      -DNO_EQUALIZER=1 \
      -DNO_8BIT=1 \
      -DNO_16BIT=1 \
      -DNO_32BIT=1 \
      -DACCURATE_ROUNDING=1 \
      -DINDEX_SIZE=0 \
      -c \
      ./src/compat/compat.c \
      ./src/libmpg123/parse.c \
  	  ./src/libmpg123/frame.c \
  	  ./src/libmpg123/format.c \
  	  ./src/libmpg123/dct64.c \
  	  ./src/libmpg123/id3.c \
  	  ./src/libmpg123/optimize.c \
  	  ./src/libmpg123/readers.c \
  	  ./src/libmpg123/tabinit.c \
  	  ./src/libmpg123/libmpg123.c \
  	  ./src/libmpg123/layer1.c \
  	  ./src/libmpg123/layer2.c \
  	  ./src/libmpg123/layer3.c \
  	  ./src/libmpg123/synth_real.c
  '';

  installPhase = ''
    mkdir -p $out/lib $out/include
    cp -rf include/* $out/include
    cp src/libmpg123/*.h $out/include
    ${wasi-sdk}/bin/llvm-ar crS $out/lib/libmpg123.a ./*.o
    ${wasi-sdk}/bin/llvm-ranlib -U $out/lib/libmpg123.a
  '';
}
