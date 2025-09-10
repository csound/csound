#!/usr/bin/env bash
source "$(dirname "$0")/nixpkgs-pin.sh"
nix-build -E '(with import <nixpkgs> {}; callPackage ./src/csound.nix { static = true; })' -o result --show-trace &&
    if [ -d "./lib" ]; then
        printf '%s\n' "Cleaning directory lib"
        rm -rf "./lib"
    fi &&
    mkdir lib &&
    cp ./result/lib/* lib &&
    chmod 0600 lib/*
