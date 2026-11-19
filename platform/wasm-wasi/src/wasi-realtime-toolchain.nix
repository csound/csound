{ pkgs, system ? pkgs.stdenv.hostPlatform.system, pkgsWasm ? pkgs.pkgsCross.wasi32 }:
# Preview 3's experimental pthreads run cooperatively. The realtime probe
# requires a native thread entry point and shared memory for parallel work.
# Strict pthread declarations keep libc's EAGAIN stubs out of the probe.
import ./wasi-preview3-toolchain.nix { inherit system pkgs; }
