#!/bin/sh

DIR=linux_d64

mkdir -p -m 0755 ../../../CSDIST/$DIR/bin
mkdir -p -m 0755 ../../../CSDIST/$DIR/doc
mkdir -p -m 0755 ../../../CSDIST/$DIR/hdr
mkdir -p -m 0755 ../../../CSDIST/$DIR/lib
mkdir -p -m 0755 ../../../CSDIST/$DIR/opc
mkdir -p -m 0755 ../../../CSDIST/$DIR/opc/frontends

rm ../../../CSDIST/$DIR/bin/*

cp -pv ../../{linseg,lpanal,brkpt,lpc_export,lpc_import,cs,makecsd,csb64enc,cscore,csound,csound5gui,cvanal,dnoise,mixer,envext,extract,extractor,pvanal,pvlook,het_export,het_import,hetro,scale,scot,scsort,sdif2ad,srconv,tabdes,winsound,cstclsh,cswish,matrix.tk,pv_export,pv_import,atsa,sndinfo,cseditor} ../../../CSDIST/$DIR/bin

strip ../../../CSDIST/$DIR/bin/*

rm ../../../CSDIST/$DIR/opc/*
rm ../../../CSDIST/$DIR/opc/frontends/*
cp -puv ../../li*.so ../../opcodes.dir ../../../CSDIST/$DIR/opc

mv ../../../CSDIST/$DIR/opc/lib_csnd.so ../../../CSDIST/$DIR/opc/frontends
mv ../../../CSDIST/$DIR/opc/lib_jcsound.so ../../../CSDIST/$DIR/opc/frontends
cp -puv ../../_loris.so ../../../CSDIST/$DIR/opc/frontends
ln ../../../CSDIST/$DIR/opc/frontends/lib_csnd.so ../../../CSDIST/$DIR/opc/frontends/_csnd.so
cp -puv ../../*.jar ../../../CSDIST/$DIR/opc/frontends
cp -puv ../../csnd.py ../../../CSDIST/$DIR/opc/frontends
cp -puv ../../loris.py ../../../CSDIST/$DIR/opc/frontends
rm ../../../CSDIST/$DIR/opc/libcsound64.so
rm ../../../CSDIST/$DIR/opc/lib_CsoundVST.so

cp -upv ../../libcsound64.so.5.1  ../../../CSDIST/$DIR/lib
(cd  ../../../CSDIST/$DIR/lib; ln -s libcsound64.so.5.1 libcsound64.so)

cp -upv ../../libcsound64.a ../../../CSDIST/$DIR/lib

cp -upv ../../H/cfgvar.h ../../H/cscore.h ../../H/csdl.h ../../H/csound.h ../../H/csound.hpp ../../H/csoundCore.h ../../H/cwindow.h ../../H/msg_attr.h ../../H/OpcodeBase.hpp ../../H/pstream.h ../../H/pvfileio.h ../../H/soundio.h ../../H/sysdep.h ../../H/text.h ../../H/version.h ../../interfaces/CsoundFile.hpp ../../interfaces/CppSound.hpp ../../interfaces/filebuilding.h ../../../CSDIST/$DIR/hdr

cp -rupv ../../../manual/html ../../../CSDIST/$DIR/doc
find ../../../CSDIST/$DIR/ -name CVS -exec rm -rf {} \;

cp -upv installer ../../../CSDIST/$DIR
cat > ../../../CSDIST/$DIR/def.ins <<'EOF'
Linux i86_64 Doubles
/usr/local/bin
/usr/local/include
/usr/local/csound64_64
/usr/local/doc/csound
/usr/local/lib64
OPCODEDIR64
libcsound64.a
'EOF'
