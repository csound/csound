#!/bin/sh

if [ "$#" == "2" ] ; then
  if [ "$1" == "remove" ] ; then
    if ( test -e "$2" ) ; then
      echo -e "Removing '$2'"
      rm -Rf "$2" ;
    fi ;
    exit 0 ;
  fi ;
  exit -1 ;
elif [ "$#" != "0" ] ; then
  exit 0 ;
fi

"$0" remove ".sconf_temp"
"$0" remove "config.log"

find . -type f -iname ".scons*" -exec "$0" remove '{}' \;
find . -type f -iname "*.xmg"   -exec "$0" remove '{}' \;
find . -type f -iname "*.o"     -exec "$0" remove '{}' \;
find . -type f -iname "*.os"    -exec "$0" remove '{}' \;
find . -type f -iname "*.so"    -exec "$0" remove '{}' \;
find . -type f -iname "*.a"     -exec "$0" remove '{}' \;
find . -type f -iname "*.wav"   -exec "$0" remove '{}' \;
find . -type f -iname "*.aif*"  -exec "$0" remove '{}' \;
find . -type f -iname "*.sf"    -exec "$0" remove '{}' \;
find . -type f -iname "*.pcm"   -exec "$0" remove '{}' \;
find . -type f -iname "*.exe"   -exec "$0" remove '{}' \;
find . -type f -iname "*.dll"   -exec "$0" remove '{}' \;
find . -type f -iname "*.obj"   -exec "$0" remove '{}' \;
find . -type f -iname "*.lib"   -exec "$0" remove '{}' \;

find . -type f -exec file '{}' \; | grep -G -e ':.*\<ELF\>' | cut -d ':' -f 1 | xargs -n 1 "$0" remove
find . -type f -exec file '{}' \; | grep -G -e ':.*\<80386\>' | cut -d ':' -f 1 | xargs -n 1 "$0" remove
find . -type f -exec file '{}' \; | grep -G -e ':.*\<PE\>' | cut -d ':' -f 1 | xargs -n 1 "$0" remove

"$0" remove "./frontends/CsoundVST/CsoundVST_wrap.cc"
"$0" remove "./frontends/CsoundVST/CsoundVST_wrap.h"
"$0" remove "./CsoundVST.py"

