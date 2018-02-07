#!/bin/sh
if [ ! -d examples-src/javascripts/ace ]
then
       git clone https://github.com/ajaxorg/ace.git --depth=1
       cp -r ace/lib/ace examples-src/javascripts/
fi

if [ ! -d examples-wasm ]
then
   mkdir examples-wasm
fi
cp -r examples-src/* examples-wasm/
cp dist-wasm/* examples-wasm/javascripts
mv examples-wasm/javascripts/libcsound.wasm examples-wasm

