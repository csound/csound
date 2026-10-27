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
nix-build ./src/wasi-preview2-support.nix \
  --impure --argstr system "$NIX_SYSTEM" --show-trace -o result_preview2_support
nix-build ./src/wasi64-support.nix \
  --impure --argstr system "$NIX_SYSTEM" --show-trace -o result_wasi64_support
nix-build ./src/csound.nix "${COMMON_ARGS[@]}" --argstr moduleKind browser -o result
# Both pointer widths use Preview 2. The x64 adapter keeps canonical calls
# in memory32 while the Csound engine and its libc use memory64.
nix-build ./src/csound-x64.nix "${COMMON_ARGS[@]}" --argstr moduleKind browser -o result_x64
nix-build ./src/csound-x64.nix "${COMMON_ARGS[@]}" --argstr moduleKind command -o result_cli_x64
nix-build ./src/csound.nix "${COMMON_ARGS[@]}" --argstr moduleKind command -o result_cli
nix-build ./src/plugin_example.nix \
  --arg sharedMemory false \
  --impure \
  --argstr system "$NIX_SYSTEM" \
  --arg csoundSdkArchive ./result/lib/csound-plugin-sdk.tar.gz \
  -o result_plugin_c \
  --show-trace
nix-build ./src/plugin_example_cpp.nix \
  --arg sharedMemory false \
  --impure \
  --argstr system "$NIX_SYSTEM" \
  --arg csoundSdkArchive ./result/lib/csound-plugin-sdk.tar.gz \
  -o result_plugin_cpp \
  --show-trace

nix-build ./src/plugin_example.nix \
  --impure --argstr system "$NIX_SYSTEM" --arg wasm64 true --arg sharedMemory false \
  --arg csoundSdkArchive ./result_x64/lib/csound_x64-plugin-sdk.tar.gz \
  -o result_plugin_c_x64 --show-trace
nix-build ./src/plugin_example_cpp.nix \
  --impure --argstr system "$NIX_SYSTEM" --arg wasm64 true --arg sharedMemory false \
  --arg csoundSdkArchive ./result_x64/lib/csound_x64-plugin-sdk.tar.gz \
  -o result_plugin_cpp_x64 --show-trace

# Replace published files only after both builds and plugin SDKs succeed.
if [ -d "./lib" ]; then
  rm -rf "./lib"
fi
mkdir lib
cp ./result_cli/lib/*.wasm ./result_cli_x64/lib/*.wasm lib
cp ./result/lib/csound.wasm ./result/lib/csound.wasm.z lib
cp ./result_x64/lib/csound_x64.wasm ./result_x64/lib/csound_x64.wasm.z lib
cp ./result/lib/csound-plugin-sdk.tar.gz lib
cp ./result/lib/csound-plugin-sdk.tar.gz lib/csound-plugin-sdk-compat.tar.gz
cp ./result_x64/lib/csound_x64-plugin-sdk.tar.gz lib
cp ./result_plugin_c/lib/plugin_example.wasm ./result_plugin_cpp/lib/plugin_example_cpp.wasm lib
cp ./result_plugin_c_x64/lib/plugin_example_x64.wasm ./result_plugin_cpp_x64/lib/plugin_example_cpp_x64.wasm lib
chmod 0644 lib/*
