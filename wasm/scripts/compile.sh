#!/usr/bin/env bash
source "$(dirname "$0")/nixpkgs-pin.sh"
GIT_HASH=$(git rev-parse HEAD 2>/dev/null || echo "HEAD")
nix-build -E "(with import <nixpkgs> {}; callPackage ./src/csound.nix { static = false; gitHash = \"$GIT_HASH\"; })" -o result --show-trace &&
nix-build -E "(with import <nixpkgs> {}; pkgs.callPackage ./src/plugin_example.nix { csoundSdkArchive = ./result/lib/csound-plugin-sdk.tar.gz; useSdkArchive = true; })" -o result_plugin_c --show-trace &&
nix-build -E "(with import <nixpkgs> {}; pkgs.callPackage ./src/plugin_example_cpp.nix { csoundSdkArchive = ./result/lib/csound-plugin-sdk.tar.gz; useSdkArchive = true; })" -o result_plugin_cpp --show-trace &&
    if [ -d "./lib" ]; then
        printf '%s\n' "Cleaning directory lib"
        rm -rf "./lib"
    fi &&
    mkdir lib &&
    cp ./result/lib/csound.wasm lib &&
    cp ./result/lib/csound.wasm.z lib &&
    cp ./result/lib/csound-plugin-sdk.tar.gz lib &&
    cp ./result_plugin_c/lib/plugin_example.wasm lib &&
    cp ./result_plugin_cpp/lib/plugin_example_cpp.wasm lib &&
    chown `whoami` lib/* &&
    chmod 0655 lib/*
