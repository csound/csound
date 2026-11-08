Csound for Xylinx Zynq FPGA board
------------

Csound can be built for the Xylinx Zynq FPGA Board. For this we supply here the cross compile and custom cmake files to
be used with the cross-compilation tools (e.g..arm-none-eabi). Typically this would involve,with the toolchain and CMake installed
in the system, from the top level source directory do

```
mkdir build
cd build
cmake .. -DCUSTOM_CMAKE=../platform/zynq/Custom.cmake -DCMAKE_TOOLCHAIN_FILE=../platform/zynq/crosscompile.cmake
make 
```

This will build the static library libcsound.a that you can use with
your Zynq project. If you want to install it to a given drectory,
use`-DCMAKE_INSTALL_PREFIX=<directory>` in the command-line above and
then

```
make install
```
