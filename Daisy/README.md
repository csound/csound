DAISY PLATFORM BUILD INSTRUCTIONS
----------

To build the Csound library for the Daisy platform, you must first install the Daisy toolchain. This includes the necessary compiler, build tools, and USB drivers required for Daisy development. You will also need to install CMake.

Once the toolchain and CMake are correctly installed on your system, open a terminal, navigate (cd) to the top-level directory of the Csound source tree, and run the following commands to configure and build the library:

```
mkdir build
cd build
cmake .. -DCUSTOM_CMAKE=../Daisy/Custom.cmake -DCMAKE_TOOLCHAIN_FILE=../Daisy/crosscompile.cmake
make 
```

This will build the static library `libcsound.a` that you can use with your Daisy C/C++ project. You can find this file in the `build` directory. If you want to install it to a given drectory, use`-DCMAKE_INSTALL_PREFIX=<directory>` in the command-line above and then

```
make install
```

Two simple example instruments are provided to demonstrate how to use the Csound library on the Daisy platform. Please refer to this [README][simple example instruments](https://github.com/csound/csound/tree/develop/Daisy/DaisyCsoundExamples/) for detailed instructions on how to build and run them on your device.


