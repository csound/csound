cp -upv ../../{linseg,lpanal,brkpt,lpc_export,lpc_import,cs,makecsd,csb64enc,cscore,csound,cvanal,dnoise,mixer,envext,extract,extractor,pvanal,pvlook,het_export,het_import,hetro,scale,scot,scsort,sdif2ad,srconv,tabdes,cstclsh,cswish} ../../../CSDIST/linux_d32/bin

cp -upv ../../li*.so ../../../CSDIST/linux_d32/opc

cp -upv ../../libcsound.a  ../../../CSDIST/linux_d32/lib

cp -rupv ../../../manual/html ../../../CSDIST/linux_d32/doc

cat > ../../../CSDIST/linux_d32/def.ins <<'EOF'
Linux Doubles
/usr/local/bin
/usr/local/csound
/usr/local/doc/csound
/usr/local/lib
OPCODEDIR64
'EOF'
