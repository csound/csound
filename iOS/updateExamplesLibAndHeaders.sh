#!/bin/sh
export OUT_DIR_OBJC=`pwd`/Csound\ iOS\ Obj-C\ Examples/csound-iOS
export OUT_DIR_SWIFT=`pwd`/Csound\ iOS\ Swift\ Examples/Csound\ iOS\ SwiftExamples/csound-iOS
export LIBSNDFILE_LIB=`pwd`/libsndfile.a 

# Assemble release
mkdir -p "$OUT_DIR_OBJC/libs"
mkdir -p "$OUT_DIR_SWIFT/libs"
cp libcsound.a "$OUT_DIR_OBJC/libs"
cp $LIBSNDFILE_LIB "$OUT_DIR_OBJC/libs"
cp libcsound.a "$OUT_DIR_SWIFT/libs"
cp $LIBSNDFILE_LIB "$OUT_DIR_SWIFT/libs"
cd ..

# COPYING HEADERS

export HEADER_DIR_OBJC="$OUT_DIR_OBJC/headers/"
echo "HEADER_DIR_OBJC: $HEADER_DIR_OBJC"
export HEADER_DIR_SWIFT="$OUT_DIR_SWIFT/headers/"
echo "HEADER_DIR_SWIFT: $HEADER_DIR_SWIFT"
cp include/*.h* "$HEADER_DIR_OBJC"
cp include/*.h* "$HEADER_DIR_SWIFT"
rm "$HEADER_DIR_OBJC/float-version.h.in"
rm "$HEADER_DIR_SWIFT/float-version.h.in"
