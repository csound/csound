#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source scripts/nixpkgs-pin.sh
nix-build src/wasi-realtime-support.nix -o result_realtime_support
mkdir -p test-artifacts
install -m644 result_realtime_support/supported test-artifacts/realtime-supported
if [ "$(cat result_realtime_support/supported)" != 1 ]; then
  rm -f test-artifacts/csound-realtime.wasm \
    test-artifacts/csound-browser-realtime.wasm.z test-artifacts/csound-probe.js \
    test-artifacts/plugin_example_shared_missing_tls.wasm \
    test-artifacts/plugin_example_shared_trap_ctor.wasm
  printf '%s\n' 'Preview 2 thread spawning is unavailable; no threaded fixtures were built.'
  exit 0
fi

# Keep the init gate and invalid plugins outside lib/, which is published.
nix-build src/csound-realtime.nix -o result_realtime
nix-build src/csound-browser-realtime.nix --arg testHooks true -o result_browser_probe
nix-build src/plugin_example.nix --arg sharedMemory true --argstr testVariant missing_tls -o result_plugin_missing_tls
nix-build src/plugin_example.nix --arg sharedMemory true --argstr testVariant trap_ctor -o result_plugin_trap_ctor
cp -f result_realtime/lib/csound-realtime.wasm test-artifacts/
cp -f result_browser_probe/lib/csound-browser-realtime.wasm.z test-artifacts/
cp -f result_plugin_missing_tls/lib/*.wasm result_plugin_trap_ctor/lib/*.wasm test-artifacts/
