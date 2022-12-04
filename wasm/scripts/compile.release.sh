#!/usr/bin/env bash

nix-build -E '(with import <nixpkgs> {}; callPackage ./src/csound.nix { static = false; })' -o result --show-trace &&
    if [ -d "./lib" ]; then
        printf '%s\n' "Cleaning directory lib"
        rm -rf "./lib"
    fi &&
    mkdir lib &&
    cp ./result/lib/* lib &&
    chown `whoami` lib/* &&
    chmod 0655 lib/* &&
    nix-build -E '(with import <nixpkgs> {}; callPackage ./src/csound.nix { static = true; })' -o result --show-trace &&
    cp ./result/lib/* lib &&
    chown `whoami` lib/* &&
    chmod 0655 lib/* &&
    printf '%s\n' "wasm binary ready in ./lib"
