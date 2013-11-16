#!/bin/sh
# script to package icsound for a website
make
mkdir icsound
cp icsound.js ./icsound
cp index.html ./icsound
cp manifest.json ./icsound
mkdir ./icsound/pnacl
mkdir ./icsound/pnacl/Release
cp -R ./pnacl/Release/icsound_unstripped.pexe ./icsound/pnacl/Release/.
cp -R ./pnacl/Release/icsound.nmf ./icsound/pnacl/Release/.
tar cf icsound.tar icsound
gzip icsound.tar

