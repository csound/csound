#!/bin/sh
mkdir examples-wasm
cp -r examples-src/* example-wasm/
cp dist-wasm/* examples-wasm/javascripts
mv examples-wasm/javascripts/libcsound.wasm examples-wasm

