# Building Csound for PNaCl

Install Google's Native Client SDK from https://developer.chrome.com/native-client/sdk/download.

Unzip the libsndfile sources into the "sndfile" directory. Run "make -f Makefile-pnacl".

Change to the nacl directory and execute "build.sh".

Change to the csound directory and execute "make".
