#!/bin/sh
export OUT_DIR=`pwd`/Csound\ iOS\ Examples/csound-iOS
export LIBSNDFILE_LIB=`pwd`/../../iOS/csound-iOS-dependencies/libsndfile.a 

# Assemble release
cp libcsound.a "$OUT_DIR/libs"
cp $LIBSNDFILE_LIB "$OUT_DIR/libs"
cd ..

# COPYING HEADERS

export HEADER_DIR="$OUT_DIR/headers"
cp H/cfgvar.h "$HEADER_DIR"
cp H/cscore.h "$HEADER_DIR"
cp H/csdl.h "$HEADER_DIR"
cp H/csound.h "$HEADER_DIR"
cp H/csound.hpp "$HEADER_DIR"
cp H/csoundCore.h "$HEADER_DIR"
cp H/cwindow.h "$HEADER_DIR"
cp H/msg_attr.h "$HEADER_DIR"
cp H/OpcodeBase.hpp "$HEADER_DIR"
cp H/pstream.h "$HEADER_DIR"
cp H/pvfileio.h "$HEADER_DIR"
cp H/soundio.h "$HEADER_DIR"
cp H/sysdep.h "$HEADER_DIR"
cp H/text.h "$HEADER_DIR"
cp H/version.h "$HEADER_DIR"
cp H/float-version.h "$HEADER_DIR"
cp interfaces/CsoundFile.hpp "$HEADER_DIR"
cp interfaces/CppSound.hpp "$HEADER_DIR"
cp interfaces/filebuilding.h "$HEADER_DIR" 
