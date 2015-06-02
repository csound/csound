#!/bin/sh
# script to package csound for a website
make
mkdir csound
cp csound.js ./csound
cp index.html ./csound
cp manifest.json ./csound
mkdir ./csound/pnacl
mkdir ./csound/pnacl/Release
cp -R ./pnacl/Release/csound_unstripped.pexe ./csound/pnacl/Release/.
cp -R ./pnacl/Release/csound.nmf ./csound/pnacl/Release/.
tar cf csound.tar csound
gzip csound.tar

