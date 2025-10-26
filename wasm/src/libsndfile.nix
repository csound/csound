{ pkgs, pkgsWasm }:

let lib = pkgs.lib;
    stdenvWasm = pkgsWasm.clang17Stdenv;
    libmpg123 = pkgs.callPackage ./libmpg123.nix { inherit pkgs pkgsWasm; };
    libflac = pkgs.callPackage ./libflac.nix { inherit pkgs pkgsWasm; };
    libogg = pkgs.callPackage ./libogg.nix { inherit pkgs pkgsWasm; };
    libvorbis = pkgs.callPackage ./libvorbis.nix { inherit pkgs pkgsWasm; };
    libopus = pkgs.callPackage ./libopus.nix { inherit pkgs pkgsWasm; };


in stdenvWasm.mkDerivation rec {
    name = "libsndfile";
    src = pkgs.fetchFromGitHub {
      owner = "libsndfile";
      repo = "libsndfile";
      rev = "52b803f57a1f4d23471f5c5f77e1a21e0721ea0e";
      sha256 = "sha256-F30k+guxVIkQouC4hCNa/galptaURlet6fsgcPaRi+g=";
    };

    buildInputs = [
      libmpg123
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
      "-I${pkgs.lame}/include"
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
      "-DENABLE_CPACK=OFF"
      "-DINSTALL_MANPAGES=OFF"
      "-DENABLE_SSE2=OFF"
    ];
}
