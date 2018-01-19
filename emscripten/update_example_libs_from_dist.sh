#!/bin/sh
mkdir examples
cp -r examples-src/* examples/
cp dist/* examples/javascripts
mv examples/javascripts/libcsound.js.mem examples

