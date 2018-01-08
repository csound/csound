#!/bin/sh
# Build command for belacsound
c++ main.cpp -o belacsound -std=c++11 -I/usr/xenomai/include -D_GNU_SOURCE \
    -D_REENTRANT -D__XENO__ -I/usr/xenomai/include/posix -I/usr/local/lib \
    -I/root/Bela/include -L/usr/local/lib `/usr/xenomai/bin/xeno-config --skin=posix --ldflags` \
    -lcsound -lsndfile -L/root/Bela/lib \
    -lbela -lbelaextra -lNE10 



