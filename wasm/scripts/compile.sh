#!/usr/bin/env bash
source "$(dirname "$0")/nixpkgs-pin.sh"
GIT_HASH=$(git rev-parse HEAD 2>/dev/null || echo "HEAD")
nix-build -E "(with import <nixpkgs> {}; callPackage ./src/csound.nix { static = false; gitHash = \"$GIT_HASH\"; })" -o result -j1 --show-trace &&
    if [ -d "./lib" ]; then
        printf '%s\n' "Cleaning directory lib"
        rm -rf "./lib"
    fi &&
    mkdir lib &&
    cp ./result/lib/csound.wasm lib &&
    cp ./result/lib/csound.wasm.z lib &&
    chown `whoami` lib/* &&
    chmod 0655 lib/*
