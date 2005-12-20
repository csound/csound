cp -upv ../../{linseg,lpanal,brkpt,lpc_export,lpc_import,cs,makecsd,csb64enc,cscore,csound,cvanal,dnoise,mixer,envext,extract,extractor,pvanal,pvlook,het_export,het_import,hetro,scale,scot,scsort,sdif2ad,srconv,tabdes,cstclsh,cswish} ../../../CSDIST/linux_f64/bin

strip ../../../CSDIST/linux_f64/bin/*

cp -upv ../../li*.so ../../../CSDIST/linux_f64/opc

cp -upv ../../libcsound.a  ../../../CSDIST/linux_f64/lib

cp -rupv ../../../manual/html ../../../CSDIST/linux_f64/doc

cat > ../../../CSDIST/linux_f64/def.ins <<'EOF'
Linux i86_64 Floats
/usr/local/bin
/usr/local/csound_64
/usr/local/doc/csound
/usr/local/lib64
OPCODEDIR
libcsound.a
'EOF'
