cd ../csound5_install
for i in CsoundLib CsoundApps SupportLibs Csound5; do
/Developer/tools/packagemaker -build -proj $i.pmproj -v -p $i.pkg
done
sh mkdistro.sh $1

