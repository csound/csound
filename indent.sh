#!/bin/bash

INDENT_OPTS="-kr -bad -bap -br -brs -c30 -cd30 -cp30 -cdw -cs -di8 -hnl -i2 -l78 -lc78 -lp -nce -nfca -npcs -nprs -npsl -nss -saf -sai -saw -st -ts2 -ut --break-after-boolean-operator"

if [ "$#" != "2" ] ; then
  echo -e "usage: $0 <infile> <outfile>"
  exit 1 ;
fi

rm -f "__indent_sh_tmpfile"

cat "$1" | expand | sed 's/[[:space:]]\+$//' |                          \
  indent ${INDENT_OPTS} |                                               \
  sed 's/^	/    /' | sed 's/	/  /g' > "__indent_sh_tmpfile"

mv -f "__indent_sh_tmpfile" "$2"

