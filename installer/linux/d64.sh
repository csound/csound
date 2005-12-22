DIR=linux_d64

cp -upv ../../{linseg,lpanal,brkpt,lpc_export,lpc_import,cs,makecsd,csb64enc,cscore,csound,cvanal,dnoise,mixer,envext,extract,extractor,pvanal,pvlook,het_export,het_import,hetro,scale,scot,scsort,sdif2ad,srconv,tabdes,cstclsh,cswish} ../../../CSDIST/$DIR/bin

strip ../../../CSDIST/$DIR/bin/*

cp -upv ../../li*.so ../../../CSDIST/$DIR/opc

cp -upv ../../libcsound.a ../../../CSDIST/$DIR/lib

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
