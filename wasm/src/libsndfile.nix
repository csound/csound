{ pkgs ? import <nixpkgs> { } }:

let wasi-sdk = pkgs.callPackage ./wasi-sdk.nix {};

in pkgs.stdenvNoCC.mkDerivation rec {
    name = "libsndfile-1.0.25";
    src = pkgs.fetchurl {
      url = "http://www.mega-nerd.com/libsndfile/files/${name}.tar.gz";
      sha256 = "10j8mbb65xkyl0kfy0hpzpmrp0jkr12c7mfycqipxgka6ayns0ar";
    };

    AR = "${wasi-sdk}/bin/ar";
    CC = "${wasi-sdk}/bin/clang";
    CPP = "${wasi-sdk}/bin/clang-cpp";
    CXX = "${wasi-sdk}/bin/clang++";
    LD = "${wasi-sdk}/bin/wasm-ld";
    NM = "${wasi-sdk}/bin/nm";
    OBJCOPY = "${wasi-sdk}/bin/objcopy";
    RANLIB = "${wasi-sdk}/bin/ranlib";

    postPatch = ''
      substituteInPlace src/sndfile.c \
        --replace 'assert (sizeof (sf_count_t) == 8) ;' ""
      substituteInPlace src/sfconfig.h \
        --replace '#include "config.h"' ""
      substituteInPlace src/common.c \
        --replace '#include	<config.h>' ""
      mv src/sndfile.h.in src/sndfile.h
      mv src/g72x.c src/g72x_parent.c
      rm src/G72x/g72x_test.c
      rm src/test_file_io.c
      find ./src -type f -exec sed -i -e 's/@TYPEOF_SF_COUNT_T@/int64_t/g' {} \;
      find ./src -type f -exec sed -i -e 's/@SIZEOF_SF_COUNT_T@/8/g' {} \;
      find ./src -type f -exec sed -i -e 's/@SF_COUNT_MAX@/0x7FFFFFFFFFFFFFFFLL/g' {} \;
    '';

    configurePhase = "true";
    dontStrip = true;
    buildPhase = ''
       ${wasi-sdk}/bin/clang \
         --sysroot=${wasi-sdk}/share/wasi-sysroot \
         --target=wasm32-unknown-emscripten \
         -fPIC -O2 -fno-omit-frame-pointer \
         -include src/sndfile.h \
         -I./src -I./src/GSM610 -I./src/G72x \
         -D__wasi__=1 \
         -D__wasm32__=1 \
         -D__EMSCRIPTEN__=0 \
         -DSIZEOF_SF_COUNT_T=8 \
         -DCPU_IS_LITTLE_ENDIAN=1 \
         -DCPU_IS_BIG_ENDIAN=0 \
         -DSIZEOF_INT64_T=8 \
         -DSIZEOF_LONG_LONG=8 \
         -DHAVE_UNISTD_H=1 \
         -DCPU_CLIPS_POSITIVE=0 \
         -DCPU_CLIPS_NEGATIVE=1 \
         -DPACKAGE_NAME='"libsndfile"' \
         -DPACKAGE_VERSION='"1.0.25"' \
         -DPACKAGE='"libsndfile"' \
         -DVERSION='"1.0.25"' \
         -DPRId64=d \
         -D_WASI_EMULATED_SIGNAL \
         -D_WASI_EMULATED_MMAN \
         -fno-exceptions -O0 -c \
         -Wno-unknown-attributes \
         -Wno-shift-op-parentheses \
         -Wno-bitwise-op-parentheses \
         -Wno-many-braces-around-scalar-init \
         -Wno-macro-redefined \
         src/G72x/*.c \
         src/GSM610/*.c \
         src/*.c
    '';

    installPhase = ''
      mkdir -p $out/lib $out/include
      rm -rf ./test*
      ${wasi-sdk}/bin/llvm-ar crS $out/lib/libsndfile.a *.o
      ${wasi-sdk}/bin/llvm-ranlib -U $out/lib/libsndfile.a
      cp src/*.h $out/include
      cp src/GSM610/*.h $out/include
      cp src/G72x/*.h $out/include
    '';
}
