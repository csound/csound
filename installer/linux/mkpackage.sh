#!/bin/sh

INSTDIR="/usr/local"
PKG_DIR="../__csound5_package"

BIN_DIR="${INSTDIR}/bin"
INC_DIR="${INSTDIR}/include"
LIB_DIR="${INSTDIR}/lib"
BINDIR2="${INSTDIR}/lib/Csound/bin"
XMG_DIR="${INSTDIR}/lib/Csound/xmg"
OPCODE_DIR="${INSTDIR}/lib/Csound/lib"
OPCOD64DIR="${INSTDIR}/lib/Csound/lib64"

UTIL_LIST_1="cvanal dnoise envext extractor het_export het_import hetro lpanal"
UTIL_LIST_1="${UTIL_LIST_1} lpc_export lpc_import mixer pvanal pvlook scale"
UTIL_LIST_1="${UTIL_LIST_1} sndinfo srconv"

UTIL_LIST_2="scsort extract cs csb64enc makecsd"

CS_HDRS="H/cfgvar.h H/cscore.h H/csdl.h H/csoundCore.h H/csound.h H/cwindow.h"
CS_HDRS="${CS_HDRS} H/msg_attr.h H/OpcodeBase.hpp H/pstream.h H/pvfileio.h"
CS_HDRS="${CS_HDRS} H/soundio.h H/sysdep.h H/text.h H/version.h"

CFLAGS="-Wall -O3 -fno-inline-functions -march=i686 -mcpu=pentium3"
CFLAGS="${CFLAGS} -fomit-frame-pointer -ffast-math"

BUILD_OPTS="buildUtilities=0 useLrint=1 noDebug=1 buildPythonOpcodes=1"
BUILD_OPTS="${BUILD_OPTS} useOSC=1 buildCsoundVST=0"

SO_VERSION="5.0"

# -----------------------------------------------------------------------------

rm -Rf "${PKG_DIR}" || exit -1

mkdir -p -m 0755 "${PKG_DIR}" || exit -1
mkdir -p -m 0755 "${PKG_DIR}/${INSTDIR}" || exit -1
mkdir -m 0755 "${PKG_DIR}/${BIN_DIR}" || exit -1
mkdir -m 0755 "${PKG_DIR}/${INC_DIR}" || exit -1
mkdir -m 0755 "${PKG_DIR}/${INC_DIR}/Csound" || exit -1
mkdir -m 0755 "${PKG_DIR}/${LIB_DIR}" || exit -1
mkdir -m 0755 "${PKG_DIR}/${LIB_DIR}/Csound" || exit -1
mkdir -m 0755 "${PKG_DIR}/${BINDIR2}" || exit -1
mkdir -m 0755 "${PKG_DIR}/${XMG_DIR}" || exit -1
mkdir -m 0755 "${PKG_DIR}/${OPCODE_DIR}" || exit -1
mkdir -m 0755 "${PKG_DIR}/${OPCOD64DIR}" || exit -1

for i in "" "64" ; do
  CS_NAME="${PKG_DIR}/${BIN_DIR}/csound${i}"
  LDLP="LD_LIBRARY_PATH"
  echo -e "#!/bin/sh"                                   > "${CS_NAME}"
  echo -e ""                                            >> "${CS_NAME}"
  echo -e "export OPCODEDIR${i}=\"${OPCODE_DIR}${i}\""  >> "${CS_NAME}"
  echo -e "export CSSTRNGS=\"${XMG_DIR}\""              >> "${CS_NAME}"
  echo -e ""                                            >> "${CS_NAME}"
  echo -e "if [ \"\${${LDLP}}\" == \"\" ] ; then"       >> "${CS_NAME}"
  echo -e "  export ${LDLP}=\"${BINDIR2}\""             >> "${CS_NAME}"
  echo -e "else"                                        >> "${CS_NAME}"
  echo -e "  export ${LDLP}=\"${BINDIR2}:\${${LDLP}}\"" >> "${CS_NAME}"
  echo -e "fi"                                          >> "${CS_NAME}"
  echo -e ""                                            >> "${CS_NAME}"
  echo -e "exec \"${BINDIR2}/csound${i}\" \"\$@\""      >> "${CS_NAME}"
  echo -e ""                                            >> "${CS_NAME}"
  chmod 755 "${CS_NAME}"
  for j in ${UTIL_LIST_1} ; do
    UTIL_NAME="${PKG_DIR}/${BIN_DIR}/${j}${i}"
    echo -e "#!/bin/sh"                                 > "${UTIL_NAME}"
    echo -e ""                                          >> "${UTIL_NAME}"
    echo -e "export OPCODEDIR${i}=\"${OPCODE_DIR}${i}\""      >> "${UTIL_NAME}"
    echo -e "export CSSTRNGS=\"${XMG_DIR}\""            >> "${UTIL_NAME}"
    echo -e ""                                          >> "${UTIL_NAME}"
    echo -e "if [ \"\${${LDLP}}\" == \"\" ] ; then"     >> "${UTIL_NAME}"
    echo -e "  export ${LDLP}=\"${BINDIR2}\""           >> "${UTIL_NAME}"
    echo -e "else"                                      >> "${UTIL_NAME}"
    echo -e "  export ${LDLP}=\"${BINDIR2}:\${${LDLP}}\""     >> "${UTIL_NAME}"
    echo -e "fi"                                        >> "${UTIL_NAME}"
    echo -e ""                                          >> "${UTIL_NAME}"
    echo -e "exec \"${BINDIR2}/csound${i}\" -U ${j} \"\$@\""  >> "${UTIL_NAME}"
    echo -e ""                                          >> "${UTIL_NAME}"
    chmod 755 "${UTIL_NAME}"
  done
done

UNINSTALL="${PKG_DIR}/${BIN_DIR}/uninstall-csound5"
echo -e "#!/bin/sh"                                     > "${UNINSTALL}"
echo -e ""                                              >> "${UNINSTALL}"
echo -e "rm -Rf \"${LIB_DIR}/Csound\""                  >> "${UNINSTALL}"
echo -e "rm -Rf \"${INC_DIR}/Csound\""                  >> "${UNINSTALL}"
for i in csound ${UTIL_LIST_1} ; do
  echo -e "rm -f \"${BIN_DIR}/${i}\""                   >> "${UNINSTALL}"
  echo -e "rm -f \"${BIN_DIR}/${i}64\""                 >> "${UNINSTALL}"
done
for i in ${UTIL_LIST_2} nsliders.tk ; do
  echo -e "rm -f \"${BIN_DIR}/${i}\""                   >> "${UNINSTALL}"
done
echo -e "rm -f \"${LIB_DIR}/libcsound.a\""              >> "${UNINSTALL}"
echo -e "rm -f \"${LIB_DIR}/libcsound64.a\""            >> "${UNINSTALL}"
echo -e "rm -f \"${LIB_DIR}/libcsound.so\""             >> "${UNINSTALL}"
echo -e "rm -f \"${LIB_DIR}/libcsound64.so\""           >> "${UNINSTALL}"
echo -e "rm -f \"${LIB_DIR}/libcsound.so.${SO_VERSION}\""   >> "${UNINSTALL}"
echo -e "rm -f \"${LIB_DIR}/libcsound64.so.${SO_VERSION}\"" >> "${UNINSTALL}"
echo -e ""                                              >> "${UNINSTALL}"
echo -e "/sbin/ldconfig"                                >> "${UNINSTALL}"
echo -e ""                                              >> "${UNINSTALL}"
echo -e "rm -f \"${BIN_DIR}/uninstall-csound5\""        >> "${UNINSTALL}"
echo -e ""                                              >> "${UNINSTALL}"
chmod 755 "${UNINSTALL}"

for i in "0" "1" ; do
  for j in "0" "1" ; do
    ./cleanup.sh
    OPTS="${BUILD_OPTS} useDouble=${i} dynamicCsoundLibrary=${j}"
    if [ "${j}" == "0" ] ; then
      scons ${OPTS} "$@" customCCFLAGS="${CFLAGS}" customCXXFLAGS="${CFLAGS}"
      if [ "${i}" == "0" ] ; then
        mv -f csound "${PKG_DIR}/${BINDIR2}/csound"
        mv -f libcsound.a "${PKG_DIR}/${LIB_DIR}/libcsound.a"
        mv -f lib*.so "${PKG_DIR}/${OPCODE_DIR}/"
      else
        mv -f csound "${PKG_DIR}/${BINDIR2}/csound64"
        mv -f libcsound.a "${PKG_DIR}/${LIB_DIR}/libcsound64.a"
        mv -f lib*.so "${PKG_DIR}/${OPCOD64DIR}/"
        for k in ${UTIL_LIST_2} ; do
          strip ${k}
          mv -f ${k} "${PKG_DIR}/${BIN_DIR}/${k}"
        done
        mv -f *.xmg "${PKG_DIR}/${XMG_DIR}/"
        cp -ai ${CS_HDRS} "${PKG_DIR}/${INC_DIR}/Csound/"
        cp -ai nsliders.tk "${PKG_DIR}/${BIN_DIR}/"
      fi
    else
      scons ${OPTS} "$@" customCCFLAGS="${CFLAGS}" libcsound.so
      if [ "${i}" == "0" ] ; then
        mv -f libcsound.so "libcsound.so.${SO_VERSION}"
        ln -s "libcsound.so.${SO_VERSION}" libcsound.so
        mv -f "libcsound.so.${SO_VERSION}" libcsound.so "${PKG_DIR}/${LIB_DIR}/"
      else
        mv -f libcsound.so "libcsound64.so.${SO_VERSION}"
        ln -s "libcsound64.so.${SO_VERSION}" libcsound64.so
        mv -f "libcsound64.so.${SO_VERSION}" "${PKG_DIR}/${LIB_DIR}/"
        mv -f libcsound64.so "${PKG_DIR}/${LIB_DIR}/"
      fi
    fi
  done
done

./cleanup.sh

strip ${PKG_DIR}/${BINDIR2}/*
strip --strip-debug ${PKG_DIR}/${LIB_DIR}/*.so.*
strip --strip-unneeded ${PKG_DIR}/${OPCODE_DIR}/lib*.so
strip --strip-unneeded ${PKG_DIR}/${OPCOD64DIR}/lib*.so

chmod 755 ${PKG_DIR}/${BIN_DIR}/*
chmod 755 ${PKG_DIR}/${BINDIR2}/*
chmod 755 ${PKG_DIR}/${LIB_DIR}/*.so*
chmod 755 ${PKG_DIR}/${OPCODE_DIR}/lib*.so
chmod 755 ${PKG_DIR}/${OPCOD64DIR}/lib*.so

chmod 644 ${PKG_DIR}/${LIB_DIR}/*.a
chmod 644 ${PKG_DIR}/${XMG_DIR}/*.xmg
chmod 644 ${PKG_DIR}/${INC_DIR}/Csound/*.h

