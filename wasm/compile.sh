#!/usr/bin/env bash

nix-build -E '(with import <nixpkgs> {}; import ./src/csound.nix)' -o result &&
    if [ -d "./lib" ]; then
        printf '%s\n' "Cleaning directory lib"
        rm -rf "./lib"
    fi &&
    mkdir lib &&
    cp ./result/lib/* lib &&
    chmod 0600 lib/*

printf '%s\n' "wasm binary ready in ./lib"
