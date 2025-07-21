# Building Csound for WebAssembly (WASM) with WASI

This guide explains how to build Csound for WebAssembly using the WASI (WebAssembly System Interface) SDK.

## Prerequisites

### macOS (using Homebrew)

1. Install LLVM with WebAssembly support:

```bash
brew install llvm
```

2. Install LLD (LLVM Linker) which includes the WebAssembly linker:

```bash
brew install lld
```

3. Install WASI libc (provides the sysroot with C headers):

```bash
brew install wasi-libc
```

4. Install WASI runtimes (provides C++ standard libraries):

```bash
brew install wasi-runtimes
```

**Important**: Both packages complement each other:

-   `wasi-libc` provides the sysroot with standard C headers (stdio.h, stdlib.h, etc.)
-   `wasi-runtimes` provides the C++ runtime libraries (libc++.a, libc++abi.a, compiler-rt.a, libunwind.a)

### Linux

1. Download and install WASI SDK from the official releases:

```bash
wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-20/wasi-sdk-20.0-linux.tar.gz
tar xzf wasi-sdk-20.0-linux.tar.gz
export WASI_SDK_PATH=/path/to/wasi-sdk-20.0
```

2. Ensure you have a recent version of Clang with WebAssembly support and LLD (LLVM linker).

Note: The WASI SDK typically includes the necessary tools including `wasm-ld`. If using a system LLVM/Clang, you may need to install LLD separately.

## Building Csound with WASI Support

### 1. Configure the build

The key to successful WASI detection is to explicitly specify the C and C++ compilers that support WebAssembly targets:

#### macOS with Homebrew:

```bash
# Get the paths
LLVM_PREFIX=$(brew --prefix llvm)
WASI_SYSROOT=$(brew --prefix wasi-libc)/share/wasi-sysroot
WASI_RUNTIMES_LIB=$(brew --prefix wasi-runtimes)/share/wasi-sysroot/lib/wasm32-wasi

# Configure with CMake (using the WASI toolchain file)
cmake -B ./build -G Ninja . \
  -DCMAKE_TOOLCHAIN_FILE=cmake/wasm32-wasi.cmake \
  -DBUILD_WASM32_WASI=ON \
  -DBUILD_TESTS=1 \
  -DCMAKE_C_COMPILER=$LLVM_PREFIX/bin/clang \
  -DCMAKE_CXX_COMPILER=$LLVM_PREFIX/bin/clang++ \
  -DCMAKE_SYSROOT=$WASI_SYSROOT \
  -DWASI_RUNTIMES_LIB=$WASI_RUNTIMES_LIB
```

**Important**: The `-DCMAKE_TOOLCHAIN_FILE=cmake/wasm32-wasi.cmake` flag is crucial for cross-compilation to WASI. Without it, CMake will try to build for your host system (macOS/Linux) instead of WebAssembly.

#### Linux with WASI SDK:

```bash
# Assuming WASI_SDK_PATH is set to your wasi-sdk installation
# Note: WASI SDK typically includes both headers and runtime libraries
cmake -B ./build -G Ninja . \
  -DCMAKE_TOOLCHAIN_FILE=cmake/wasm32-wasi.cmake \
  -DBUILD_WASM32_WASI=ON \
  -DBUILD_TESTS=1 \
  -DCMAKE_C_COMPILER=$WASI_SDK_PATH/bin/clang \
  -DCMAKE_CXX_COMPILER=$WASI_SDK_PATH/bin/clang++ \
  -DCMAKE_SYSROOT=$WASI_SDK_PATH/share/wasi-sysroot \
  -DWASI_RUNTIMES_LIB=$WASI_SDK_PATH/share/wasi-sysroot/lib/wasm32-wasi
```

### 2. Build the WASM target

Once configured, build the WebAssembly target:

```bash
cmake --build ./build --target csound_wasm
```

This will build both the WASM dependencies (libsndfile) and the main `libcsound.wasm` executable that includes all libcsound functionality with the WASI interface from `Top/main_wasm.c`.

## Understanding the WASI Compilation Process

When building for WASI, the compilation uses both `wasi-libc` and `wasi-runtimes`:

1. **Headers from wasi-libc**: The compiler finds standard C headers (stdio.h, stdlib.h, etc.) in the wasi-libc sysroot
2. **Libraries from wasi-runtimes**: The linker finds C++ runtime libraries (libc++.a, libc++abi.a) in the wasi-runtimes directory

Example of manual compilation showing how both are used:

```bash
# For C++ code
clang++ --target=wasm32-wasi \
        --sysroot="$(brew --prefix wasi-libc)/share/wasi-sysroot" \
        -L"$(brew --prefix wasi-runtimes)/share/wasi-sysroot/lib/wasm32-wasi" \
        main.cpp -lc++ -lc++abi -o main.wasm

# For C code (only needs wasi-libc)
clang --target=wasm32-wasi \
      --sysroot="$(brew --prefix wasi-libc)/share/wasi-sysroot" \
      main.c -o main.wasm
```

## Troubleshooting

### WASI not detected

If you see the message "WASI _not_ detected", check the following:

1. **Verify compiler path**: Ensure the clang compiler you're using supports WebAssembly:

    ```bash
    $(brew --prefix llvm)/bin/clang --print-targets | grep wasm
    ```

    You should see `wasm32` in the output.

2. **Clear CMake cache**: If you've run CMake before, clear the cache:

    ```bash
    rm -rf ./build/CMakeCache.txt ./build/CMakeFiles
    ```

3. **Check sysroot**: Verify the WASI sysroot exists:

    ```bash
    ls $(brew --prefix wasi-libc)/share/wasi-sysroot
    ```

    If this directory doesn't exist, install wasi-libc:

    ```bash
    brew install wasi-libc
    ```

4. **Check C++ runtime libraries**: Verify the WASI runtime libraries exist:
    ```bash
    ls $(brew --prefix wasi-runtimes)/share/wasi-sysroot/lib/wasm32-wasi
    ```
    You should see files like `libc++.a`, `libc++abi.a`. If not, install wasi-runtimes:
    ```bash
    brew install wasi-runtimes
    ```

### Common Issues

1. **Missing wasm-ld linker**: If you see the error `clang: error: unable to execute command: Executable "wasm-ld" doesn't exist!`, you need to install LLD:

    ```bash
    # macOS
    brew install lld

    # Linux (if not included with WASI SDK)
    # Install via your package manager, e.g.:
    sudo apt install lld  # Debian/Ubuntu
    sudo dnf install lld  # Fedora
    ```

2. **CMake picks wrong compiler**: CMake caches compiler detection. Always specify `-DCMAKE_C_COMPILER` and `-DCMAKE_CXX_COMPILER` explicitly.

3. **PATH modifications don't work**: Simply modifying PATH before running CMake is not reliable because:

    - CMake has its own search order for compilers
    - It caches the results in CMakeCache.txt
    - It may prefer system compilers over PATH

4. **Missing WASI sysroot**: The WASI sysroot provides the necessary headers and libraries for WebAssembly. Without it, compilation will fail even with a compatible compiler.

5. **Missing C++ standard library**: If you see linker errors about missing `-lc++` or `-lc++abi`, ensure you have installed `wasi-runtimes` and provided the `WASI_RUNTIMES_LIB` path to CMake.

## Using Custom.cmake

Alternatively, you can set the compiler paths in `Custom.cmake`:

```cmake
set(BUILD_WASM32_WASI ON)

# For macOS with Homebrew
set(CMAKE_C_COMPILER "/opt/homebrew/opt/llvm/bin/clang" CACHE STRING "C compiler" FORCE)
set(CMAKE_CXX_COMPILER "/opt/homebrew/opt/llvm/bin/clang++" CACHE STRING "C++ compiler" FORCE)

# Set the WASI runtime libraries path (adjust based on your system)
set(WASI_RUNTIMES_LIB "/opt/homebrew/opt/wasi-runtimes/share/wasi-sysroot/lib/wasm32-wasi" CACHE PATH "WASI runtime libraries path")
```

Then run CMake without the compiler flags:

```bash
WASI_SDK_PATH=$(brew --prefix wasi-libc)/share/wasi-sysroot \
cmake -B ./build -G Ninja . -DCMAKE_SYSROOT="$WASI_SDK_PATH"
```

Or if not using Custom.cmake, provide the toolchain file and paths:

```bash
cmake -B ./build -G Ninja . \
  -DCMAKE_TOOLCHAIN_FILE=cmake/wasm32-wasi.cmake \
  -DCMAKE_SYSROOT="$(brew --prefix wasi-libc)/share/wasi-sysroot" \
  -DWASI_RUNTIMES_LIB="$(brew --prefix wasi-runtimes)/share/wasi-sysroot/lib/wasm32-wasi"
```

## Verifying the Build

After a successful build, you should see:

-   "WASI detected successfully" during configuration
-   "Building libsndfile.a for WASM32 WASI target" message
-   The `csound_wasm` target available for building

The resulting WebAssembly module can be used in web browsers or any WASI-compatible runtime.
