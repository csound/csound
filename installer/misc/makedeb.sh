#!/bin/sh

if [ "`whoami`" != "root" ] ; then
    echo -e "Error: this script should be run as root"
    exit -1 ;
fi

umask 022

rm -f "./debian/files"
rm -Rf "./debian/csound"
rm -Rf "./debian/tmp"

mkdir -p -m 0755 "./debian/tmp"
mkdir -p -m 0755 "./debian/tmp/DEBIAN"

for i in ./debian/tmp/DEBIAN/postinst ./debian/tmp/DEBIAN/postrm ; do
    echo -e "#\041/bin/sh"                                  >  "${i}"
    echo -e ""                                              >> "${i}"
    if [ "${i}" == "./debian/tmp/DEBIAN/postinst" ] ; then
        echo -e "if [ \"\$1\" == \"configure\" ] ; then"    >> "${i}" ;
    else
        echo -e "if [ \"\$1\" == \"remove\" ] ; then"       >> "${i}" ;
    fi
    echo -e "    /sbin/ldconfig ;"                          >> "${i}"
    echo -e "fi"                                            >> "${i}"
    echo -e ""                                              >> "${i}"
    echo -e "exit 0"                                        >> "${i}"
    echo -e ""                                              >> "${i}"
    chmod 0755 "${i}" ;
done

SAVED_CWD="`pwd`"
cd "../__csound6"
find . \! -type d -print | cpio -p -d -m -C 1048576 "${SAVED_CWD}/debian/tmp"
cd "${SAVED_CWD}"
find "./debian/tmp" -exec chown -h 0:0 "{}" \;
find "./debian/tmp" -type d -exec chmod 0755 "{}" \;
find "./debian/tmp" -type f -perm /0100 -exec chmod 0755 "{}" \;
find "./debian/tmp" -type f \! -perm +0100 -exec chmod 0644 "{}" \;

dh_makeshlibs
dh_gencontrol
dh_md5sums
dh_builddeb

rm -f "./debian/files"
rm -Rf "./debian/csound"
rm -Rf "./debian/tmp"

