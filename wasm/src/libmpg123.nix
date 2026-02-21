{
  pkgs,
  pkgsWasm,
  stdenvWasm,
}: let
  lib = pkgs.lib;
in
  stdenvWasm.mkDerivation rec {
    name = "mpg123";
    version = "1.33.3";

    nativeBuildInputs = [
      pkgs.pkg-config
    ];

    src = pkgs.fetchurl {
      url = "mirror://sourceforge/${name}/${name}-${version}.tar.bz2";
      sha256 = "sha256-agxkct0VbiE8IGj0ARXru3OXjC2HPma64qJQ4tIZjSY=";
    };

    configureFlags = [
      "--disable-components"
      "--enable-libmpg123"
      "--host=wasm32-wasi"
      "--libdir=${placeholder "out"}/lib"
      "--with-pkgconfigdir=${placeholder "out"}/lib/pkgconfig"
    ];

    dontStrip = true;
  }
