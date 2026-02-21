{
  pkgs ? import <nixpkgs> {},
  pkgsWasm ? pkgs.pkgsCross.wasi32,
  stdenvWasm ? pkgsWasm.clangStdenv,
  csoundSdkArchive ? ../lib/csound-plugin-sdk.tar.gz,
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
  else pkgs.callPackage ./csound.nix {inherit pkgs pkgsWasm stdenvWasm;}
