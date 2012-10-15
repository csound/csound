cd ../csound6_install
for i in CsoundLib64 CsoundApps64 SupportLibs Csound6-doubles; do
/Developer/tools/packagemaker -build -proj $i.pmproj -v -p $i.pkg
done
sh mkdistro64.sh $1

