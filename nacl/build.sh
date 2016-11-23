#!/bin/sh

flex -B -t ../Engine/csound_orc.lex > csound_orclex.c 
flex -B ../Engine/csound_pre.lex > csound_prelex.c 
bison -d -pcsound_orc --report=itemset -o csound_orcparse.c ../Engine/csound_orc.y

# Actually build Csound.
export TOOLCHAIN=pnacl
make -j 6


