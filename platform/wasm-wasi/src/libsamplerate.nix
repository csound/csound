{
  pkgs,
  stdenvWasm,
}:
stdenvWasm.mkDerivation {
  pname = "libsamplerate-wasi";
  inherit (pkgs.libsamplerate) version src;
  nativeBuildInputs = [pkgs.cmake pkgs.ninja];
  cmakeFlags = [
    "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
    # This release prepends the install prefix in samplerate.pc itself.
    "-DCMAKE_INSTALL_LIBDIR=lib"
    "-DCMAKE_INSTALL_INCLUDEDIR=include"
    "-DBUILD_SHARED_LIBS=OFF"
    "-DBUILD_TESTING=OFF"
    "-DLIBSAMPLERATE_EXAMPLES=OFF"
    "-DLIBSAMPLERATE_INSTALL=ON"
  ];
}
