#!/bin/sh
if [ ! -d examples-src/javascripts/ace ]
   then
       git clone https://github.com/ajaxorg/ace.git
       mkdir examples-src/javascripts/ace
       cp -r ace/lib/ace examples-src/javascripts/ace/
fi

if [ ! -d examples-wasm ]
 then
   mkdir examples-wasm
fi
cp -r examples-src/* examples-wasm/
cp dist-wasm/* examples-wasm/javascripts
mv examples-wasm/javascripts/libcsound.wasm examples-wasm

