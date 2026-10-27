{ pkgs, system ? pkgs.stdenv.hostPlatform.system, pkgsWasm ? pkgs.pkgsCross.wasi32 }:
# Preview 2 has no pthread spawning ABI. The strict pthread declarations in
# this toolchain make the capability probe fail instead of linking libc stubs
# which return EAGAIN. Keep the probe so a future toolchain can enable it.
import ./wasi-preview2-toolchain.nix { inherit system pkgs; }
