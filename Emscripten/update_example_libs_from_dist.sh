#!/bin/sh
if [ ! -d examples-src/javascripts/ace ]
   then
       git clone https://github.com/ajaxorg/ace.git
       mkdir examples-src/javascripts/ace
       cp -r ace/lib/ace examples-src/javascripts/ace/
fi

if [ ! -d examples ]
 then
     mkdir examples
fi
cp -r examples-src/* examples/
cp dist/* examples/javascripts


