{
  system ? builtins.currentSystem,
  pkgs ? import <nixpkgs> {inherit system;},
  pkgsWasm ? pkgs.pkgsCross.wasi32,
  stdenvWasm ? pkgsWasm.clangStdenv,
  # Match the browser binary selected from this package at build time.
  sharedMemory ? builtins.pathExists ../lib/csound-browser-realtime.wasm.z,
  csoundSdkArchive ? if sharedMemory
    then ../lib/csound-plugin-sdk.tar.gz
    else ../lib/csound-plugin-sdk-compat.tar.gz,
  useSdkArchive ? true,
}: let
  archiveAvailable = useSdkArchive && builtins.pathExists csoundSdkArchive;
in
  if archiveAvailable
  then
    stdenvWasm.mkDerivation {
      pname = "csound-wasm-plugin-sdk";
      version = "0.0.0";
      dontUnpack = true;
      dontConfigure = true;
      dontBuild = true;

      nativeBuildInputs = [pkgs.buildPackages.gnutar];

      installPhase = ''
        mkdir -p $out
        tar -xzf ${csoundSdkArchive} -C $out
        if [ -d "$out/csound-plugin-sdk" ]; then
          mv "$out/csound-plugin-sdk/"* "$out/"
          rmdir "$out/csound-plugin-sdk"
        fi
      '';
    }
  else if sharedMemory
  then
    pkgs.callPackage ./csound-browser-realtime.nix { inherit pkgs; }
  else
    pkgs.callPackage ./csound.nix {
      inherit pkgs pkgsWasm stdenvWasm;
      moduleKind = "browser";
    }
