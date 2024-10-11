DAISY PLATFORM BUILD INSTRUCTIONS
----------

To build the Csound library for the Daisy platform, with the toolchain installed and CMake,
from the top level source directory do

```
mkdir build
cd build
cmake .. -DCUSTOM_CMAKE=../daisy/Custom.cmake -DCMAKE_TOOLCHAIN_FILE=../daisy/crosscompile.cmake
make 
```

This will build the static library libcsound.a that you can use with
your Daisy C/C++ project. If you want to install it to a given drectory,
use`-DCMAKE_INSTALL_PREFIX=<directory>` in the command-line above and
then

```
make install
```

