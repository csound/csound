#!/bin/sh
# Build command for belacsound
c++ main.cpp -o belacsound -std=c++11 -fno-pie -no-pie `/usr/xenomai/bin/xeno-config --skin=posix --cflags` -I/usr/local/lib \
    -I/root/Bela/include -L/usr/local/lib `/usr/xenomai/bin/xeno-config --skin=posix --ldflags` \
    -lcsound -lsndfile -L/root/Bela/lib \
    -lbela -lbelaextra -lNE10 



