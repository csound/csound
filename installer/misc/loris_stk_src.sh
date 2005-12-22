#!/bin/bash

LORIS_VERSION="1.3.0"
STK_VERSION="4.2.1"

# -----------------------------------------------------------------------------

if [ "$#" != "1" ] || [ "$1" != "unpack" ] && [ "$1" != "clean" ] ; then
    echo -e ""
    echo -e "Usage:"
    echo -e ""
    echo -e "    $0 unpack"
    echo -e ""
    echo -e "        Unpack Loris and STK sources."
    echo -e "        loris-${LORIS_VERSION}.tar.gz and"
    echo -e "        stk-${STK_VERSION}.tar.gz should be in the"
    echo -e "        parent directory."
    echo -e ""
    echo -e "    $0 clean"
    echo -e ""
    echo -e "        Remove Loris and STK sources."
    echo -e ""
    exit 1 ;
fi

cd "./Opcodes/Loris/" || exit 1
rm -Rf "loris-${LORIS_VERSION}" "scripting" "src"
if [ "$1" == "unpack" ] ; then
    tar -xpzvf "../../../loris-${LORIS_VERSION}.tar.gz" || exit 1
    mv -f "loris-${LORIS_VERSION}/scripting" ./
    mv -f "loris-${LORIS_VERSION}/src" ./
    rm -Rf "loris-${LORIS_VERSION}" ;
fi
cd ../../

cd "./Opcodes/stk/" || exit 1
rm -Rf "stk-${STK_VERSION}" "include" "rawwaves" "src"
if [ "$1" == "unpack" ] ; then
    tar -xpzvf "../../../stk-${STK_VERSION}.tar.gz" || exit 1
    mv -f "stk-${STK_VERSION}/include" ./
    mv -f "stk-${STK_VERSION}/rawwaves" ./
    mv -f "stk-${STK_VERSION}/src" ./
    rm -Rf "stk-${STK_VERSION}" ;
fi
cd ../../

