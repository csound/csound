# Building Csound for PNaCl

Install Google's Native Client SDK from xxx.

In this directory, there is a sndfile directory containing byteswap.h and a makefile. Copy the source files from the 
Android build of libsndfile into the nacl/sndfile directory, and run "make" there.

Change to the nacl directory and execute "build.sh".

Change to the csound directory and execute "make".
