#!/bin/sh
# script to package csound for a website
#make
mkdir csound
cp csound.js ./csound
cp *.html ./csound
cp *.csd ./csound
cp sf_GMbank.sf2 ./csound
cp manifest.json ./csound
mkdir ./csound/pnacl
mkdir ./csound/pnacl/Release
cp -R ./pnacl/Release/csound.pexe ./csound/pnacl/Release/.
cp -R ./pnacl/Release/csound.nmf ./csound/pnacl/Release/.
tar cf csound6.05-pnacl.tar csound
gzip  csound6.05-pnacl.tar
rm -r csound


