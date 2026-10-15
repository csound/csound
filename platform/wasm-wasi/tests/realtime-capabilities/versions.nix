{ rev ? "4db2c220f32fd162658ed1b7bb2f46a82996ddbe" }:
let
  pkgs = import (builtins.fetchTarball
    "https://github.com/NixOS/nixpkgs/archive/${rev}.tar.gz") { };
  wasm = pkgs.pkgsCross.wasi32;
  llvmVersion = name:
    if builtins.hasAttr name pkgs then pkgs.${name}.llvm.version else null;
in
{
  inherit rev;
  system = builtins.currentSystem;
  clangWasm = wasm.clangStdenv.cc.version;
  wasiLibc = wasm.wasilibc.version;
  wasiLibcSource = wasm.wasilibc.src.rev or null;
  llvmDefault = pkgs.llvmPackages.llvm.version;
  llvm22 = llvmVersion "llvmPackages_22";
  llvm23 = llvmVersion "llvmPackages_23";
  wasmtime = pkgs.wasmtime.version;
  paths = {
    clang = toString wasm.clangStdenv.cc;
    clangUnwrapped = toString wasm.clangStdenv.cc.cc;
    lld = toString pkgs.llvmPackages.lld;
    llvm = toString pkgs.llvmPackages.llvm;
    libc = toString wasm.wasilibc;
  };
}
