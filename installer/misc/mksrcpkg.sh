#!/bin/bash

if [ "$#" != "1" ] ; then
    echo -e "Usage: $0 <version>"
    echo -e "       (example: $0 5.00.0)"
    exit 1 ;
fi

PKGDIR="../__csound6_src/Csound${1}"
rm -Rf "../__csound6_src"
mkdir -p -m 0755 "../__csound6_src"
mkdir -p -m 0755 "${PKGDIR}"

cp -aiLR * "${PKGDIR}/"
SAVED_PWD="`pwd`"
cd "${PKGDIR}"

./installer/misc/loris_stk_src.sh clean
./cleanup.sh
find . -type f -wholename "*/CVS/*" -exec rm -f "{}" \;
find . -type d -empty -exec rmdir -p "{}" \;
find . -type d -exec chmod 755 "{}" \;
find . -type f -perm /0100 -exec chmod 755 "{}" \;
find . -type f \! -perm +0100 -exec chmod 644 "{}" \;
find . -type f -iname "*.[ch]" -exec chmod 644 "{}" \;
find . -type f -iname "*.[ch]pp" -exec chmod 644 "{}" \;
find . -type f -iname "*.[ch]xx" -exec chmod 644 "{}" \;
find . -type f -iname "*.cc" -exec chmod 644 "{}" \;
find . -type f -iname "*.java" -exec chmod 644 "{}" \;
find . -type f -iname "*.csd" -exec chmod 644 "{}" \;
find . -type f -iname "*.orc" -exec chmod 644 "{}" \;
find . -type f -iname "*.sco" -exec chmod 644 "{}" \;
find . -type f -iname "*.sh" -exec chmod 755 "{}" \;
find strings -type f -exec chmod 644 "{}" \;

cd ..
tar -c -v --group=0 --owner=0 -f "Csound${1}.tar" "Csound${1}"
chmod 644 "Csound${1}.tar"
gzip -9 "Csound${1}.tar"
zip -9 -r -X -o "Csound${1}.zip" "Csound${1}"

cd "${SAVED_PWD}"

