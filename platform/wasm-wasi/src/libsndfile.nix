{
  pkgs,
  pkgsWasm,
  stdenvWasm,
}: let
  lib = pkgs.lib;
  libmpg123 = pkgs.callPackage ./libmpg123.nix {inherit pkgs pkgsWasm stdenvWasm;};
  liblame = pkgs.callPackage ./liblame.nix {inherit pkgs pkgsWasm stdenvWasm;};
  libflac = pkgs.callPackage ./libflac.nix {inherit pkgs pkgsWasm stdenvWasm;};
  libogg = pkgs.callPackage ./libogg.nix {inherit pkgs pkgsWasm stdenvWasm;};
  libvorbis = pkgs.callPackage ./libvorbis.nix {inherit pkgs pkgsWasm stdenvWasm;};
  libopus = pkgs.callPackage ./libopus.nix {inherit pkgs pkgsWasm stdenvWasm;};
in
  stdenvWasm.mkDerivation rec {
    name = "libsndfile";
    src = pkgs.fetchFromGitHub {
      owner = "libsndfile";
      repo = "libsndfile";
      rev = "52b803f57a1f4d23471f5c5f77e1a21e0721ea0e";
      sha256 = "sha256-F30k+guxVIkQouC4hCNa/galptaURlet6fsgcPaRi+g=";
    };

    buildInputs = [
      libmpg123
      liblame
      libogg
      libvorbis
      libflac
      libopus
    ];

    nativeBuildInputs = [
      pkgs.cmake
      pkgs.pkg-config
    ];

    dontStrip = true;

    NIX_CFLAGS_COMPILE = [
      "-I${pkgs.libogg.dev}/include"
      "-I${pkgs.flac.dev}/include"
      "-I${pkgs.libvorbis.dev}/include"
      "-I${liblame}/include"
      "-I${libmpg123}/include"
    ];

    cmakeFlags = [
      "-DCMAKE_BUILD_TYPE=RELEASE"
      "-DBUILD_SHARED_LIBS=OFF"
      "-DBUILD_PROGRAMS=OFF"
      "-DBUILD_EXAMPLES=OFF"
      "-DBUILD_TESTING=OFF"
      "-DENABLE_EXTERNAL_LIBS=ON"
      "-DENABLE_MPEG=ON"
      "-DMP3LAME_INCLUDE_DIR=${liblame}/include"
      "-DMP3LAME_LIBRARY=${liblame}/lib/libmp3lame.a"
      "-DMPG123_INCLUDE_DIR=${libmpg123}/include"
      "-DMPG123_LIBRARY=${libmpg123}/lib/libmpg123.a"
      "-DENABLE_CPACK=OFF"
      "-DINSTALL_MANPAGES=OFF"
      "-DENABLE_SSE2=OFF"
    ];
  }
