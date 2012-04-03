#!/bin/sh
export RELEASE_DIR=csound-iOS-5.17.3
export LIBSNDFILE_LIB=`pwd`/../../iOS/csound-iOS-dependencies/libsndfile.a 

rm -rf $RELEASE_DIR
mkdir $RELEASE_DIR

# Build Documentation
cd docs
pdflatex csound_ios_manual.tex

# Assemble release
cd ../$RELEASE_DIR
cp ../docs/csound_ios_manual.pdf .

# COPYING HEADERS

cp -R ../Csound\ iOS\ Examples .
