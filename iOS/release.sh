#!/bin/sh
export RELEASE_DIR=csound-iOS-6.11.0

#remove backup files ending with ~
find . -name "*~" -exec rm {} \;

rm -rf $RELEASE_DIR
mkdir $RELEASE_DIR

# Build Documentation
cd docs
pdflatex csound_ios_manual.tex

# Assemble release
cd ../$RELEASE_DIR
cp ../docs/csound_ios_manual.pdf .

# COPYING HEADERS

cp -R ../Csound\ iOS\ Obj-C\ Examples .
cp -R ../Csound\ iOS\ Swift\ Examples .
cp Csound\ iOS\ Obj-C\ Examples/LICENSE.TXT .
cp ../CHANGELOG .

cd ..
zip -r ${RELEASE_DIR}.zip ${RELEASE_DIR}
