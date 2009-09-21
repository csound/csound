cd ../csound5_install
for i in CsoundLib CsoundApps SupportLibs Csound5; do
/Developer/usr/bin/packagemaker --doc $i.pmdoc $i.mpkg
done
sh mkdistro.sh $1

