#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source scripts/nixpkgs-pin.sh

# Keep the init gate and invalid plugins outside lib/, which is published.
nix-build src/csound-realtime.nix --no-out-link
nix-build src/csound-browser-realtime.nix --arg testHooks true -o result_browser_probe
nix-build src/plugin_example.nix --arg sharedMemory true --argstr testVariant missing_tls -o result_plugin_missing_tls
nix-build src/plugin_example.nix --arg sharedMemory true --argstr testVariant trap_ctor -o result_plugin_trap_ctor
mkdir -p test-artifacts
cp -f result_browser_probe/lib/csound-browser-realtime.wasm.z test-artifacts/
cp -f result_plugin_missing_tls/lib/*.wasm result_plugin_trap_ctor/lib/*.wasm test-artifacts/
