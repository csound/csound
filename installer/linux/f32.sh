rm ../../../CSDIST/linux_f32/bin/*

cp -pv ../../{linseg,lpanal,brkpt,lpc_export,lpc_import,cs,makecsd,csb64enc,cscore,csound,cvanal,dnoise,mixer,envext,extract,extractor,pvanal,pvlook,het_export,het_import,hetro,scale,scot,scsort,sdif2ad,srconv,tabdes,cstclsh,cswish} ../../../CSDIST/linux_f32/bin

strip ../../../CSDIST/linux_f32/bin/*

rm ../../../CSDIST/linux_f32/opc/*
cp -pv ../../li*.so ../../*.jar ../../_csnd.so ../../../CSDIST/linux_f32/opc

cp -upv ../../libcsound.a  ../../../CSDIST/linux_f32/lib

cp -rupv ../../../manual/html ../../../CSDIST/linux_f32/doc
find ../../../CSDIST/linux_f32/ -name CVS -exec rm -rf {} \;

cat > ../../../CSDIST/linux_f32/def.ins <<'EOF'
Linux Floats
/usr/local/bin
/usr/local/csound
/usr/local/doc/csound
/usr/local/lib
OPCODEDIR
libcsound.a
'EOF'
