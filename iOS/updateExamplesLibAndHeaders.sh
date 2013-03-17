#!/bin/sh
export OUT_DIR=`pwd`/Csound\ iOS\ Examples/csound-iOS
export LIBSNDFILE_LIB=`pwd`/../../iOS/csound-iOS-dependencies/libsndfile.a 

# Assemble release
cp libcsound.a "$OUT_DIR/libs"
cp $LIBSNDFILE_LIB "$OUT_DIR/libs"
cd ..

# COPYING HEADERS

export HEADER_DIR="$OUT_DIR/headers/"
echo "HEADER_DIR: $HEADER_DIR"
cp include/cfgvar.h "$HEADER_DIR"
cp include/cscore.h "$HEADER_DIR"
cp include/csdl.h "$HEADER_DIR"
cp include/csound.h "$HEADER_DIR"
cp include/csound.hpp "$HEADER_DIR"
cp include/csoundCore.h "$HEADER_DIR"
cp include/cwindow.h "$HEADER_DIR"
cp include/msg_attr.h "$HEADER_DIR"
cp include/OpcodeBase.hpp "$HEADER_DIR"
cp include/pstream.h "$HEADER_DIR"
cp include/pvfileio.h "$HEADER_DIR"
cp include/soundio.h "$HEADER_DIR"
cp include/sysdep.h "$HEADER_DIR"
cp include/text.h "$HEADER_DIR"
cp include/version.h "$HEADER_DIR"
cp include/float-version.h "$HEADER_DIR"
cp interfaces/CsoundFile.hpp "$HEADER_DIR"
cp interfaces/CppSound.hpp "$HEADER_DIR"
cp interfaces/filebuilding.h "$HEADER_DIR" 
