#!/bin/bash

INSTDIR="/usr/local"
VERSION=`date "+%Y%m%d"`
MD5FILE="${INSTDIR}/Csound5-${VERSION}.md5sums"
CS_PERM=0755
EXEPERM=0755
LIBPERM=0755
XMGPERM=0644

EXEFILES=`find . -type f -maxdepth 1 \! -iname "*.so" -print | xargs -n 1000 file | grep -e "\<ELF\>" | cut -d ":" -f 1`
LIBFILES=`find . -type f -maxdepth 1 -iname "*.so" -print | xargs -n 1000 file | grep -e "\<ELF\>" | cut -d ":" -f 1`
XMGFILES=`find . -type f -maxdepth 1 -iname "*.xmg" -print`

echo -e "Csound5 Linux installer by Istvan Varga\n"

rm -f "${MD5FILE}" || exit -1
mkdir -p -m 0755 "${INSTDIR}/bin" || exit -1
mkdir -p -m 0755 "${INSTDIR}/lib/Csound" || exit -1
mkdir -p -m 0755 "${INSTDIR}/share/Csound" || exit -1

echo -e " === Installing executables ==="
for i in ${EXEFILES} ; do
  j="${INSTDIR}/bin/`basename ${i}`"
  echo -e "${j}"
  rm -f "${j}" || exit -1
  cp -af "${i}" "${j}" || exit -1
  strip --strip-unneeded "${j}"
  chown 0:0 "${j}" 2> /dev/null
  if [ "${j}" == "${INSTDIR}/bin/csound" ] ; then
    chmod $CS_PERM "${j}" ;
  else
    chmod $EXEPERM "${j}" ;
  fi
  md5sum -b "${j}" >> "${MD5FILE}" ;
done
echo -e " === Installing plugins ==="
for i in ${LIBFILES} ; do
  j="${INSTDIR}/lib/Csound/`basename ${i}`"
  echo -e "${j}"
  rm -f "${j}" || exit -1
  cp -af "${i}" "${j}" || exit -1
  strip --strip-debug "${j}"
  chown 0:0 "${j}" 2> /dev/null
  chmod $LIBPERM "${j}" ;
  md5sum -b "${j}" >> "${MD5FILE}" ;
done
echo -e " === Installing string databases ==="
for i in ${XMGFILES} ; do
  j="${INSTDIR}/share/Csound/`basename ${i}`"
  echo -e "${j}"
  rm -f "${j}" || exit -1
  cp -af "${i}" "${j}" || exit -1
  chown 0:0 "${j}" 2> /dev/null
  chmod $XMGPERM "${j}" ;
  md5sum -b "${j}" >> "${MD5FILE}" ;
done

chown 0:0 "${MD5FILE}" 2> /dev/null
chmod 644 "${MD5FILE}" || exit -1

echo -e "\nCsound5 installation is complete.\n"
echo -e "To run Csound, please set the following environment variables:"
echo -e "  OPCODEDIR=${INSTDIR}/lib/Csound"
echo -e "  CSSTRNGS=${INSTDIR}/share/Csound"
echo -e "  CS_LANG=uk, CS_LANG=us, or CS_LANG=fr to select language"
echo -e "To test the integrity of the Csound5 installation, type:"
echo -e "  md5sum --check ${MD5FILE}"
echo -e "Csound5 can be uninstalled with the following commands:"
echo -e "  rm -f \`cat ${MD5FILE} | cut -d \"*\" -f 2\`"
echo -e "  rm -f ${MD5FILE}\n"

