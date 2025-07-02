#!/usr/bin/env bash

nix-build -E '(with import <nixpkgs> {}; callPackage ./src/csound.nix { })' -o result --show-trace &&
    if [ -d "./lib" ]; then
        printf '%s\n' "Cleaning directory lib"
        rm -rf "./lib"
    fi &&
    mkdir lib &&
    cp ./result/lib/* lib &&
    chmod 0600 lib/* &&
    nix-build -E '(with import <nixpkgs> {}; pkgs.callPackage ./src/plugin_example.nix {})' -o result &&
    cp ./result/lib/* lib &&
    # chmod 0600 lib/* &&
    # nix-build -E '(with import <nixpkgs> {}; pkgs.callPackage ./src/plugin_example_cxx.nix {})' -o result &&
    # cp ./result/lib/* lib &&
    printf '%s\n' "wasm binary ready in ./lib"
