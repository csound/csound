#!/bin/sh
export OUT_DIR=`pwd`/Csound\ iOS\ Examples/csound-iOS
export LIBSNDFILE_LIB=`pwd`/../../iOS/csound-iOS-dependencies/libsndfile.a 

# Assemble release
mkdir -p "$OUT_DIR/libs"
cp libcsound.a "$OUT_DIR/libs"
cp $LIBSNDFILE_LIB "$OUT_DIR/libs"
cd ..

# COPYING HEADERS

export HEADER_DIR="$OUT_DIR/headers/"
echo "HEADER_DIR: $HEADER_DIR"
cp include/*.h* "$HEADER_DIR"
rm "$HEADER_DIR/float-version.h.in"
