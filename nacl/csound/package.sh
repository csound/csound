#!/bin/sh
# script to package csound for a website
#make
mkdir  csound6.08-pnacl
cp csound.js ./csound6.08-pnacl
cp *.html ./csound6.08-pnacl
cp *.csd ./csound6.08-pnacl
cp sf_GMbank.sf2 ./csound6.08-pnacl
cp manifest.json ./csound6.08-pnacl
mkdir ./csound6.08-pnacl/csound
cp -R ./pnacl/ ./csound6.08-pnacl/csound/..
tar cf csound6.08-pnacl.tar csound6.08-pnacl
gzip  csound6.08-pnacl.tar
rm -r csound6.08-pnacl


