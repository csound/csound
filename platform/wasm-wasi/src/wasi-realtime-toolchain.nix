{ pkgs, pkgsWasm ? pkgs.pkgsCross.wasi32 }:
let
  libc = pkgsWasm.wasilibc.override { enablePosixThreads = true; };
  cCompiler = pkgsWasm.clangStdenv.cc.override (old: {
    inherit libc;
    bintools = pkgsWasm.clangStdenv.cc.bintools.override { inherit libc; };
    extraBuildCommands = (old.extraBuildCommands or "") + ''
      echo " -pthread" >> "$out/nix-support/cc-cflags"
    '';
  });
  cStdenv = pkgs.overrideCC pkgsWasm.clangStdenv cCompiler;
  # The default WASI libc++ and libc++abi disable thread support. Rebuild both,
  # rather than putting a threaded allocator behind a single-thread C++ runtime.
  libcxx = (pkgsWasm.llvmPackages.libcxx.override {
    stdenv = cStdenv;
    devExtraCmakeFlags = [
      "-DLIBCXX_ENABLE_THREADS=ON"
      "-DLIBCXXABI_ENABLE_THREADS=ON"
      "-DLIBCXX_HAS_PTHREAD_API=ON"
    ];
  });
  compiler = cCompiler.override { inherit libcxx; };
in {
  inherit libc libcxx compiler;
  stdenv = pkgs.overrideCC pkgsWasm.clangStdenv compiler;
}
