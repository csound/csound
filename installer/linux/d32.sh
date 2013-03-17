
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

cp -upv ../../libcsound64.so.5.2  ../../../CSDIST/$DIR/lib
(cd  ../../../CSDIST/$DIR/lib; ln -s libcsound64.so.5.2 libcsound64.so)

cp -upv ../../libcsound64.a  ../../../CSDIST/$DIR/lib

cp -upv ../../include/cfgvar.h ../../include/cscore.h ../../include/csdl.h ../../include/csound.h ../../include/csound.hpp ../../include/csoundCore.h ../../include/cwindow.h ../../include/msg_attr.h ../../include/OpcodeBase.hpp ../../include/pstream.h ../../include/pvfileio.h ../../include/soundio.h ../../include/sysdep.h ../../include/text.h ../../include/version.h ../../interfaces/CsoundFile.hpp ../../interfaces/CppSound.hpp ../../interfaces/filebuilding.h ../../../CSDIST/$DIR/hdr

cp -rupv ../../../manual/html ../../../CSDIST/$DIR/doc
find ../../../CSDIST/$DIR/ -name CVS -exec rm -rf {} \;

###en_GB/  es_CO/    fr/  it/ ro/  de/   en_US/ /ru /ro
cp -upv ../../po/de/LC_MESSAGES/csound5.mo ../../../CSDIST/$DIR/loc/de_csound5.mo
cp -upv ../../po/en_GB/LC_MESSAGES/csound5.mo ../../../CSDIST/$DIR/loc/en_csound5.mo
cp -upv ../../po/en_US/LC_MESSAGES/csound5.mo ../../../CSDIST/$DIR/loc/us_csound5.mo
cp -upv ../../po/es_CO/LC_MESSAGES/csound5.mo ../../../CSDIST/$DIR/loc/es_csound5.mo
cp -upv ../../po/fr/LC_MESSAGES/csound5.mo ../../../CSDIST/$DIR/loc/fr_csound5.mo
cp -upv ../../po/it/LC_MESSAGES/csound5.mo ../../../CSDIST/$DIR/loc/it_csound5.mo
cp -upv ../../po/ro/LC_MESSAGES/csound5.mo ../../../CSDIST/$DIR/loc/ro_csound5.mo
cp -upv ../../po/ru/LC_MESSAGES/csound5.mo ../../../CSDIST/$DIR/loc/ru_csound5.mo

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
