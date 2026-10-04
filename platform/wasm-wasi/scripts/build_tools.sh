#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
source "$SCRIPT_DIR/nixpkgs-pin.sh"
cd "$SCRIPT_DIR/.."

if [ -z "${NIX_SYSTEM:-}" ]; then
  NIX_SYSTEM=$(nix-instantiate --eval --expr builtins.currentSystem)
  NIX_SYSTEM=${NIX_SYSTEM#\"}
  NIX_SYSTEM=${NIX_SYSTEM%\"}
fi

nix-build ./src/tools.nix \
  --impure \
  --argstr system "$NIX_SYSTEM" \
  --argstr gitHash "$(git rev-parse HEAD 2>/dev/null || echo none)" \
  --argstr buildDate "$(LC_ALL=C date '+%Y-%m-%d')" \
  --show-trace \
  -o result_tools

# lib is the release directory published by @csound/wasm-bin.
# Preserve the Csound modules and plugin SDK from compile.sh.
mkdir -p lib
cp result_tools/lib/*.wasm lib/
chmod 0644 lib/*.wasm
