
        #!/bin/sh

DIR=linux_d32

mkdir -p -m 0755 ../../../CSDIST/$DIR/bin
mkdir -p -m 0755 ../../../CSDIST/$DIR/doc
mkdir -p -m 0755 ../../../CSDIST/$DIR/hdr
mkdir -p -m 0755 ../../../CSDIST/$DIR/lib
mkdir -p -m 0755 ../../../CSDIST/$DIR/opc
mkdir -p -m 0755 ../../../CSDIST/$DIR/opc/frontends
mkdir -p -m 0755 ../../../CSDIST/$DIR/loc

rm ../../../CSDIST/$DIR/bin/*

cp -pv ../../{linseg,lpanal,brkpt,lpc_export,lpc_import,cs,makecsd,csb64enc,cscore,csound,csound5gui,cvanal,dnoise,mixer,envext,extract,extractor,pvanal,pvlook,het_export,het_import,hetro,scale,scot,scsort,sdif2ad,srconv,tabdes,winsound,cstclsh,cswish,matrix.tk,pv_export,pv_import,atsa,sndinfo} ../../../CSDIST/$DIR/bin

strip ../../../CSDIST/$DIR/bin/*

rm ../../../CSDIST/$DIR/opc/*
rm ../../../CSDIST/$DIR/opc/frontends/*

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

cp -upv ../../libcsound64.a  ../../../CSDIST/$DIR/lib

cp -upv ../../H/cfgvar.h ../../H/cscore.h ../../H/csdl.h ../../H/csound.h ../../H/csound.hpp ../../H/csoundCore.h ../../H/cwindow.h ../../H/msg_attr.h ../../H/OpcodeBase.hpp ../../H/pstream.h ../../H/pvfileio.h ../../H/soundio.h ../../H/sysdep.h ../../H/text.h ../../H/version.h ../../interfaces/CsoundFile.hpp ../../interfaces/CppSound.hpp ../../interfaces/filebuilding.h ../../../CSDIST/$DIR/hdr

cp -rupv ../../../manual/html ../../../CSDIST/$DIR/doc
find ../../../CSDIST/$DIR/ -name CVS -exec rm -rf {} \;

###en_GB/  es_CO/    fr/  ro/  de/   en_US/
cp -upv ../../po/de/LC_MESSAGES/csound5.mo ../../../CSDIST/$DIR/loc
cp -upv ../../po/en_GB/LC_MESSAGES/csound5.mo ../../../CSDIST/$DIR/loc
cp -upv ../../po/en_US/LC_MESSAGES/csound5.mo ../../../CSDIST/$DIR/loc
cp -upv ../../po/es_CO/LC_MESSAGES/csound5.mo ../../../CSDIST/$DIR/loc
cp -upv ../../po/fr/LC_MESSAGES/csound5.mo ../../../CSDIST/$DIR/loc
cp -upv ../../po/ro/LC_MESSAGES/csound5.mo ../../../CSDIST/$DIR/loc

cp -upv installer ../../../CSDIST/$DIR

cat > ../../../CSDIST/$DIR/def.ins <<'EOF'
Linux Doubles
/usr/local/bin
/usr/local/include
/usr/local/csound
/usr/local/doc/csound
/usr/local/lib
/usr/local/share/locale
OPCODEDIR64
libcsound64.a
'EOF'
