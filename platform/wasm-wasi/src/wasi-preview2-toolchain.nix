{ system ? builtins.currentSystem, pkgs ? import <nixpkgs> { inherit system; } }:
let
  target = "wasm32-wasip2";
  libcSource = pkgs.callPackage ./wasi-libc-source.nix {};
  pkgsWasm = import pkgs.path {
    localSystem = { inherit system; };
    config.replaceCrossStdenv = { buildPackages, baseStdenv }:
      baseStdenv.override { cc = buildPackages.llvmPackages_23.clang; };
    crossSystem = {
      system = "wasm32-wasi";
      useLLVM = true;
      # CMake calls this platform WASI, including for Preview 2.
      uname = { system = "WASI"; processor = "wasm32"; release = null; };
    };
    overlays = [(final: prev: {
      # Reuse native Nix tools; only the WASI runtimes need cross builds.
      llvmPackages_23 = prev.llvmPackages_23.overrideScope (_: super:
        pkgs.lib.optionalAttrs (!prev.stdenv.hostPlatform.isWasm) {
          inherit (pkgs.llvmPackages_23) libllvm libclang lld;
        } // pkgs.lib.optionalAttrs prev.stdenv.hostPlatform.isWasm {
          libcxx = super.libcxx.overrideAttrs (old: {
            patches = (old.patches or []) ++ [./libcxx-wasi-locale.patch];
          });
        });
      # Keep Darwin's native bootstrap unchanged while using LLVM 23 for WASI.
      llvmPackages = if prev.stdenv.targetPlatform.isWasm
        then final.llvmPackages_23 else prev.llvmPackages;
      wasilibc = prev.wasilibc.overrideAttrs (old: {
        inherit (libcSource) src version;
        # Nixpkgs' tool-version patch targets an older libc. Use the checked-in
        # bindings below; their optional regeneration tools are not built.
        patches = [];
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
      echo " --target=${target} -D_WASI_STRICT_PTHREAD -fuse-ld=${pkgs.llvmPackages_23.lld}/bin/wasm-ld" \
        >> "$out/nix-support/cc-cflags"
    '';
  });
in {
  inherit pkgsWasm compiler target;
  libc = pkgsWasm.wasilibc;
  stdenv = pkgs.overrideCC pkgsWasm.clangStdenv compiler;
}
