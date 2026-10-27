{ system ? builtins.currentSystem, pkgs ? import <nixpkgs> { inherit system; } }:
let
  target = "wasm64-wasip2";
  pkgsWasm = import pkgs.path {
    localSystem = { inherit system; };
    crossSystem = {
      system = "wasm64-wasi";
      useLLVM = true;
      uname = { system = "WASI"; processor = "wasm64"; release = null; };
    };
    overlays = [(final: prev: {
      # LLVM lowers Wasm floating comparisons to i32 even on wasm64. The
      # generic compiler-rt LP64 rule returns long/i64 and creates trap stubs.
      wasi64Builtins = prev.llvmPackages.compiler-rt.overrideAttrs (old: {
        postPatch = (old.postPatch or "") + ''
          substituteInPlace lib/builtins/fp_compare_impl.inc \
            --replace-fail '#if defined(__aarch64__) || defined(__arm64ec__)' \
              '#if defined(__aarch64__) || defined(__arm64ec__) || defined(__wasm__)'
        '';
      });
      wasilibc = prev.wasilibc.overrideAttrs (old: {
        pname = "wasilibc-preview2-memory64";
        nativeBuildInputs = old.nativeBuildInputs ++ [pkgs.python3];
        postPatch = (old.postPatch or "") + ''
          python3 ${../scripts/wasm64/patch-libc.py}
        '';
        cmakeFlags = builtins.filter
          (flag: !(pkgs.lib.hasPrefix "-DTARGET_TRIPLE=" flag) && !(pkgs.lib.hasPrefix "-DBUILTINS_LIB=" flag))
          old.cmakeFlags ++ [
            "-DTARGET_TRIPLE=${target}"
            "-DBUILTINS_LIB=${final.wasi64Builtins}/lib/wasi/libclang_rt.builtins-wasm64.a"
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
      cp -rL "$out/resource-root/lib" "$out/resource-root/lib-preview2"
      chmod u+w "$out/resource-root/lib-preview2"
      rm "$out/resource-root/lib"
      mv "$out/resource-root/lib-preview2" "$out/resource-root/lib"
      mkdir -p "$out/resource-root/lib/wasm64-unknown-wasip2"
      ln -s ${pkgsWasm.wasi64Builtins}/lib/wasi/libclang_rt.builtins-wasm64.a \
        "$out/resource-root/lib/wasm64-unknown-wasip2/libclang_rt.builtins.a"
      echo " --target=${target} -D_WASI_STRICT_PTHREAD -fuse-ld=${pkgs.llvmPackages.lld}/bin/wasm-ld" \
        >> "$out/nix-support/cc-cflags"
    '';
  });
in {
  inherit pkgsWasm compiler target;
  libc = pkgsWasm.wasilibc;
  stdenv = pkgs.overrideCC pkgsWasm.clangStdenv compiler;
}
