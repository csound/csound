# Building Csound for PNaCl

Install Google's Native Client SDK from https://developer.chrome.com/native-client/sdk/download. Be sure to set the environment variable NACL_SDK_ROOT to the appropriate pepper directory, e.g. `export NACL_SDK_ROOT=/home/mkg/nacl_sdk/pepper_49`.

Unzip the libsndfile sources into the "sndfile" directory. Run `make -f Makefile-pnacl`.

Change to the nacl directory and execute "build.sh".

Change to the csound directory and execute "make".
