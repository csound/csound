LIBCSOUND build instructions - arm Cortex M7
----------

To build the Csound library for the arm Cortex-m7 platform from these sources, you must first install the arm-none-eabi cross-compile toolchain. This can be installed
in one of two ways

- From the Arm Developer site: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads - this includes the latest released version of the compiler toolchain. 
- With the Daisy toolchain: https://daisy.audio/tutorials/cpp-dev-env/#1-install-the-toolchain this includes the compiler toolchain (not necessarily the latest) and other build tools, as well USB drivers required for Daisy development. This is recommended if you are planning to use Csound on the Daisy platform.

You will also need to install CMake: https://cmake.org

With these, on the terminal at top level of the Csound source tree, and run the following commands to configure and build the library:

```
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=../Daisy -DCUSTOM_CMAKE=../Daisy/Custom.cmake -DCMAKE_TOOLCHAIN_FILE=../Daisy/crosscompile.cmake
make 
```

This will build the static library `libcsound.a` that you can use with your Daisy C/C++ project, and the command

```
make install
```

installs the library and required headers under the Daisy/lib and Daisy/include directories of the Csound source tree. 
