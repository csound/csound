#!/bin/bash

mkdir csound-mingw64-i686
cd csound-mingw64-i686 
cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../Toolchain-mingw64-i686.cmake -DCMAKE_INSTALL_PREFIX=dist -DCUSTOM_CMAKE=../Custom-mingw64-i686.cmake
make -j6 install

