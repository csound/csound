# Building Csound for PNaCl

Install Google's Native Client SDK from https://developer.chrome.com/native-client/sdk/download. Be sure to set the environment variable `NACL_SDK_ROOT` to the appropriate pepper directory, e.g. `export NACL_SDK_ROOT=/home/mkg/nacl_sdk/pepper_49`.

Unzip the libsndfile sources into the `csound/nacl/sndfile`. directory. In the '`csound/nacl/sndfile/src` directory, execute `make -f Makefile-pnacl`.

Change to the `csound/nacl` directory and execute `./build.sh`.

Change to the `csound/nacl/csound` directory and execute `make`.
