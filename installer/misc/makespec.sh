#!/bin/sh

rm -f "csound.spec"
cd ..
TOPDIR=`pwd`
cd -
cat "installer/misc/csound.spec.in" | sed "s#@TOPDIR@#${TOPDIR}#g" > "csound.spec"
find "../__csound6" \! -type d -print | sort | sed 's/^\.\.\/__csound6//' >> "csound.spec"

