{ pkgs }:
# LLVM 23's Preview 3 backend and linker hard-code i32 task-context signatures.
# The generated wasm64 instructions already use i64; keep those signatures in
# step with the pointer width until upstream supplies the same fix.
pkgs.llvmPackages_23.overrideScope (_: prev: {
  libllvm = prev.libllvm.overrideAttrs (old: {
    patches = (old.patches or []) ++ [./llvm-wasm64-task-context.patch];
  });
  lld = prev.lld.overrideAttrs (old: {
    patches = (old.patches or []) ++ [./lld-wasm64-task-context.patch];
  });
})
