#!/bin/sh
if [ ! -d examples ]
 then
     mkdir examples
fi
cp -r examples-src/* examples/
cp dist/* examples/javascripts
mv examples/javascripts/libcsound.js.mem examples

