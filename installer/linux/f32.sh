DIR=linux_f32

rm ../../../CSDIST/$DIR/bin/*

cp -pv ../../{linseg,lpanal,brkpt,lpc_export,lpc_import,cs,makecsd,csb64enc,cscore,csound,cvanal,dnoise,mixer,envext,extract,extractor,pvanal,pvlook,het_export,het_import,hetro,scale,scot,scsort,sdif2ad,srconv,tabdes,cstclsh,cswish} ../../../CSDIST/$DIR/bin

strip ../../../CSDIST/$DIR/bin/*

rm ../../../CSDIST/$DIR/opc/*
rm ../../../CSDIST/$DIR/opc/frontends/*
cp -puv ../../li*.so ../../../CSDIST/$DIR/opc

mv ../../../CSDIST/$DIR/opc/lib_csnd.so ../../../CSDIST/$DIR/opc/frontends
ln ../../../CSDIST/$DIR/opc/frontends/lib_csnd.so ../../../CSDIST/$DIR/opc/frontends/_csnd.so 
mv ../../../CSDIST/$DIR/opc/lib_jcsound.so ../../../CSDIST/$DIR/opc/frontends
cp -puv ../../*.jar ../../../CSDIST/$DIR/opc/frontends
cp -puv ../../csnd.py ../../../CSDIST/$DIR/opc/frontends
rm ../../../CSDIST/$DIR/opc/libcsound.so

cp -upv ../../libcsound.so.5.1  ../../../CSDIST/$DIR/lib
(cd  ../../../CSDIST/$DIR/lib; ln -s libcsound.so.5.1 libcsound.so)

cp -upv ../../libcsound.a  ../../../CSDIST/$DIR/lib

cp -upv ../../H/cfgvar.h ../../H/cscore.h ../../H/csdl.h ../../H/csound.h ../../H/csound.hpp ../../H/csoundCore.h ../../H/cwindow.h ../../H/msg_attr.h ../../H/OpcodeBase.hpp ../../H/pstream.h ../../H/pvfileio.h ../../H/soundio.h ../../H/sysdep.h ../../H/text.h ../../H/version.h ../../interfaces/CsoundFile.hpp ../../interfaces/CppSound.hpp ../../interfaces/filebuilding.h ../../../CSDIST/$DIR/hdr

cp -rupv ../../../manual/html ../../../CSDIST/$DIR/doc
find ../../../CSDIST/$DIR/ -name CVS -exec rm -rf {} \;

cp -upv installer ../../../CSDIST/$DIR
cat > ../../../CSDIST/$DIR/def.ins <<'EOF'
Linux Floats
/usr/local/bin
/usr/local/include
/usr/local/csound
/usr/local/doc/csound
/usr/local/lib
OPCODEDIR
libcsound.a
'EOF'
