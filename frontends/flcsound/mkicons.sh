#! /bin/sh
# Construct flCsound xbm icons.

make LDLIBS=-lm px2pt

for mag in 1 2 3 4 5 6 7 8 9 16
do
  size=`expr 16 '*' $mag`
  resolution=`./px2pt $mag`
  echo "\mag=$mag; input flcsound" | mf
  gftopk flcsound.${resolution}gf
  pk2bm -cO flcsound.${resolution}pk > flcsound${size}.xbm \
      || echo pk2bm return code is $?
  echo created flcsound${size}.xbm
done

# Convert each to xpm's and then
#
# sed 's/#FFFFFF/None/' flcsound?*.xpm > flcsound.xpm 

