{ system ? builtins.currentSystem, pkgs ? import <nixpkgs> { inherit system; } }:
let
  target = "wasm32-wasip2";
  pkgsWasm = import pkgs.path {
    localSystem = { inherit system; };
    crossSystem = {
      system = "wasm32-wasi";
      useLLVM = true;
      # CMake calls this platform WASI, including for Preview 2.
      uname = { system = "WASI"; processor = "wasm32"; release = null; };
    };
    overlays = [(final: prev: {
      wasilibc = prev.wasilibc.overrideAttrs (old: {
        pname = "wasilibc-preview2";
        cmakeFlags = builtins.filter
          (flag: !(pkgs.lib.hasPrefix "-DTARGET_TRIPLE=" flag))
          old.cmakeFlags ++ [
            "-DTARGET_TRIPLE=${target}"
            "-DBUILD_SHARED=OFF"
            "-DBINDINGS_TARGET=OFF"
          ];
        passthru = (old.passthru or {}) // {
          incdir = "/include/${target}";
          libdir = "/lib/${target}";
        };
        preFixup = "";
      });
    })];
  };
  compiler = pkgsWasm.clangStdenv.cc.override (old: {
    extraBuildCommands = (old.extraBuildCommands or "") + ''
      # Nix's compiler runtime contains no WASI calls, but Clang selects its
      # path from the full target triple. Give this wrapper a private alias.
      cp -rL "$out/resource-root/lib" "$out/resource-root/lib-preview2"
      chmod u+w "$out/resource-root/lib-preview2"
      rm "$out/resource-root/lib"
      mv "$out/resource-root/lib-preview2" "$out/resource-root/lib"
      mkdir -p "$out/resource-root/lib/wasm32-unknown-wasip2"
      ln -s ${pkgsWasm.llvmPackages.compiler-rt}/lib/wasi/libclang_rt.builtins-wasm32.a \
        "$out/resource-root/lib/wasm32-unknown-wasip2/libclang_rt.builtins.a"
      # Build core modules first. Commands become components after linking;
      # browser reactors retain their C API and use a Preview 2 host.
      echo " --target=${target} -D_WASI_STRICT_PTHREAD -fuse-ld=${pkgs.llvmPackages.lld}/bin/wasm-ld" \
        >> "$out/nix-support/cc-cflags"
    '';
  });
in {
  inherit pkgsWasm compiler target;
  libc = pkgsWasm.wasilibc;
  stdenv = pkgs.overrideCC pkgsWasm.clangStdenv compiler;
}
