#!/bin/sh
# Build command for belacsound
c++ main.cpp -o belacsound -I/usr/local/lib -I/root/Bela/include -L/usr/local/lib -lcsound -lsndfile -L/root/Bela/lib \
    -lbela -lbelaextra -Wl,--no-as-needed -Wl,@/usr/xenomai/lib/cobalt.wrappers -Wl,@/usr/xenomai/lib/modechk.wrappers \
    -L/usr/xenomai/lib -lcobalt -lmodechk -lpthread -lrt


