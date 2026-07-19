#!/usr/bin/env bash
set -euo pipefail

source "$(dirname "$0")/nixpkgs-pin.sh"
GIT_HASH=$(git rev-parse HEAD 2>/dev/null || echo "none")
BUILD_DATE=$(LC_ALL=C date "+%Y-%m-%d")
if [ -z "${NIX_SYSTEM:-}" ]; then
  # --raw is unavailable in older Nix releases. Plain evaluation prints a
  # quoted string, which can be normalized with shell parameter expansion.
  NIX_SYSTEM=$(nix-instantiate --eval --expr builtins.currentSystem)
  NIX_SYSTEM=${NIX_SYSTEM#\"}
  NIX_SYSTEM=${NIX_SYSTEM%\"}
fi

COMMON_ARGS=(
  --impure
  --argstr system "$NIX_SYSTEM"
  --argstr gitHash "$GIT_HASH"
  --argstr buildDate "$BUILD_DATE"
  --show-trace
)

# The selected system controls where Nix builds these derivations. Set
# NIX_SYSTEM to a remote builder's system when a non-native build is desired.
nix-build ./src/csound.nix "${COMMON_ARGS[@]}" --argstr moduleKind browser -o result
nix-build ./src/csound.nix "${COMMON_ARGS[@]}" --argstr moduleKind command -o result_cli
nix-build ./src/plugin_example.nix \
  --impure \
  --argstr system "$NIX_SYSTEM" \
  --arg csoundSdkArchive ./result/lib/csound-plugin-sdk.tar.gz \
  -o result_plugin_c \
  --show-trace
nix-build ./src/plugin_example_cpp.nix \
  --impure \
  --argstr system "$NIX_SYSTEM" \
  --arg csoundSdkArchive ./result/lib/csound-plugin-sdk.tar.gz \
  -o result_plugin_cpp \
  --show-trace

if [ -d "./lib" ]; then
    printf '%s\n' "Cleaning directory lib"
    rm -rf "./lib"
fi
mkdir lib
cp ./result_cli/lib/csound.wasm lib
cp ./result/lib/csound-no-entry.wasm lib
cp ./result/lib/csound-no-entry.wasm.z lib
cp ./result/lib/csound-plugin-sdk.tar.gz lib
cp ./result_plugin_c/lib/plugin_example.wasm lib
cp ./result_plugin_cpp/lib/plugin_example_cpp.wasm lib
chmod 0644 lib/*
