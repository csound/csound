let hostPkgs = (import <nixpkgs> {});
    wasi-sdk = hostPkgs.callPackage ./wasi-sdk.nix {};
in
with import <nixpkgs> {
  config = { allowUnsupportedSystem = true; };
  crossSystem = {
    config = "wasm32-unknown-wasi";
    libc = "wasilibc";
    useLLVM = true;
  };
};

clangStdenv.mkDerivation rec {

    name = "libsndfile-1.0.25";
    src = fetchurl {
      url = "http://www.mega-nerd.com/libsndfile/files/${name}.tar.gz";
      sha256 = "10j8mbb65xkyl0kfy0hpzpmrp0jkr12c7mfycqipxgka6ayns0ar";
    };

    buildInputs = [ wasi-sdk ];

    nativeBuildInputs = [
      hostPkgs.autoconf
      hostPkgs.automake
      hostPkgs.python
      hostPkgs.libtool
      hostPkgs.pkgconfig
      hostPkgs.autogen
    ];

    postPatch = ''
      substituteInPlace src/sndfile.c \
        --replace 'assert (sizeof (sf_count_t) == 8) ;' ""
    '';

    NIX_CFLAGS_COMPILE = toString [
      "--sysroot=${wasi-sdk}/share/wasi-sysroot"
      "-I${wasi-sdk}/share/wasi-sysroot/include"
      "-DHAVE_UNISTD_H=1"
      "-DHAVE_READ=1"
      "-DHAVE_PREAD=1"
    ];

    # patches = [ ../../c/snfile.patch ];

    configureFlags = [
      "--disable-external-libs"
      "--enable-static"
      "--disable-shared"
      "--host=wasm64"
      "--prefix=$out"
      "--includedir=$out/include"
      "--libdir=$out/lib"
    ];

    buildPhase = ''
      ./configure ${lib.strings.concatStringsSep " " configureFlags}
      make
      make install
    '';

    outputs = [ "out" ];

}
