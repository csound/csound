#!/usr/bin/env bash
source "$(dirname "$0")/nixpkgs-pin.sh"
GIT_HASH=$(git rev-parse HEAD 2>/dev/null || echo "HEAD")
nix-build -E "(with import <nixpkgs> {}; callPackage ./src/csound.nix { static = true; gitHash = \"$GIT_HASH\"; })" -o result --show-trace &&
    if [ -d "./lib" ]; then
        printf '%s\n' "Cleaning directory lib"
        rm -rf "./lib"
    fi &&
    mkdir lib &&
    cp ./result/lib/* lib &&
    chmod 0600 lib/*
