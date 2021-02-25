{ pkgs ? import <nixpkgs> { }, static ? false }:

let lib = pkgs.lib;
    wasi-sdk-dyn = pkgs.callPackage ./wasi-sdk.nix { };
    wasi-sdk-static = pkgs.callPackage ./wasi-sdk-static.nix { };
    wasi-sdk = if static then wasi-sdk-static else wasi-sdk-dyn;

in pkgs.stdenvNoCC.mkDerivation rec {
    name = "libsndfile";
    src = pkgs.fetchFromGitHub {
      owner = "libsndfile";
      repo = "libsndfile";
      rev = "4fe1c6a0b370625bc4878aa24e0c1ad6fbc77072";
      sha256 = "1pn7bv577hbgl6ncflc23xkkrjjlljp0rbl5agffaiq7qalfva7d";
    };

    AR = "${wasi-sdk}/bin/ar";
    CC = "${wasi-sdk}/bin/clang";
    CPP = "${wasi-sdk}/bin/clang-cpp";
    CXX = "${wasi-sdk}/bin/clang++";
    LD = "${wasi-sdk}/bin/wasm-ld";
    NM = "${wasi-sdk}/bin/nm";
    OBJCOPY = "${wasi-sdk}/bin/objcopy";
    RANLIB = "${wasi-sdk}/bin/ranlib";

    patches = [ ./libsndfile_disable_oggopus.patch ];

    postPatch = ''
      # substituteInPlace src/sndfile.c \
      #   --replace 'assert (sizeof (sf_count_t) == 8) ;' ""
      mv include/sndfile.h.in include/sndfile.h
      mv src/g72x.c src/g72x_parent.c
      rm src/G72x/g72x_test.c
      rm src/test_file_io.c
      rm src/ogg_opus.c
      find ./ -type f -exec sed -i -e 's/@TYPEOF_SF_COUNT_T@/int64_t/g' {} \;
      find ./ -type f -exec sed -i -e 's/@SIZEOF_SF_COUNT_T@/8/g' {} \;
      find ./ -type f -exec sed -i -e 's/@SF_COUNT_MAX@/0x7FFFFFFFFFFFFFFFLL/g' {} \;
    '';

    configurePhase = "true";
    dontStrip = true;

    buildPhase = ''
       ${wasi-sdk}/bin/clang \
         --sysroot=${wasi-sdk}/share/wasi-sysroot \
         ${lib.optionalString (static == false) "--target=wasm32-unknown-emscripten" } \
         ${lib.optionalString (static == false) "-fPIC" } \
         -O3 \
         -include include/sndfile.h \
         -I./include -I./src -I./src/ALAC \
         -I./src/GSM610 -I./src/G72x \
         -I${pkgs.libogg.dev}/include \
         -I${pkgs.flac.dev}/include \
         -I${pkgs.libvorbis.dev}/include \
         -D__wasi__=1 \
         -D__wasm32__=1 \
         -DOS_IS_WIN32=0 \
         -DUSE_WINDOWS_API=0 \
         -DSIZEOF_SF_COUNT_T=8 \
         -DCPU_IS_LITTLE_ENDIAN=1 \
         -DCPU_IS_BIG_ENDIAN=0 \
         -DSIZEOF_INT64_T=8 \
         -DSIZEOF_LONG_LONG=8 \
         -DHAVE_UNISTD_H=1 \
         -DCPU_CLIPS_POSITIVE=0 \
         -DCPU_CLIPS_NEGATIVE=1 \
         -DHAVE_EXTERNAL_XIPH_LIBS=1 \
         -DPACKAGE_NAME='"libsndfile"' \
         -DPACKAGE_VERSION='"1.0.25"' \
         -DPACKAGE='"libsndfile"' \
         -DVERSION='"1.0.25"' \
         -D_WASI_EMULATED_SIGNAL \
         -D_WASI_EMULATED_MMAN \
         -fno-exceptions -c \
         -Wno-unknown-attributes \
         -Wno-shift-op-parentheses \
         -Wno-bitwise-op-parentheses \
         -Wno-many-braces-around-scalar-init \
         -Wno-macro-redefined \
         src/G72x/*.c \
         src/GSM610/*.c \
         src/ALAC/*.c \
         src/*.c
    '';

    installPhase = ''
      mkdir -p $out/lib $out/include
      rm -rf ./test*
      ${wasi-sdk}/bin/llvm-ar crS $out/lib/libsndfile.a *.o
      ${wasi-sdk}/bin/llvm-ranlib -U $out/lib/libsndfile.a
      cp include/*.h $out/include
      cp src/*.h $out/include
      cp src/GSM610/*.h $out/include
      cp src/G72x/*.h $out/include
    '';
}
