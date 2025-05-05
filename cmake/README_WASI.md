# WASM32 WASI Support for Csound

This directory contains support for building Csound for WebAssembly using the WASI (WebAssembly System Interface) target.

## Building libsndfile.a for WASM32 WASI

The CMake build system has been extended to support building libsndfile.a for WASM32 WASI. This is a prerequisite for building csound.wasm.

### Requirements

-   A C compiler with WASI support (e.g., Clang 13+)
-   WASI SDK installed and available in your PATH or specified via the WASI_SDK_PATH environment variable
-   CMake 3.13.4 or higher

### Building

1. Configure CMake with the `USE_WASM32_WASI` option enabled:

```bash
cmake -DUSE_WASM32_WASI=ON /path/to/csound/source
```

2. Build the WASM32 WASI target:

```bash
cmake --build . --target csound_wasm
```

This will:

1. Check if your compiler supports WASI
2. If supported, build libsndfile.a for WASM32 WASI
3. The resulting library will be placed in `${CMAKE_BINARY_DIR}/wasm32-wasi/lib/libsndfile.a`

### Using the WASM32 WASI libsndfile.a

The libsndfile.a archive built for WASM32 WASI can be used as input to link together a final csound.wasm file. This allows for debugging and verification before integrating into the full Csound WASM build.

## Debugging

If you encounter issues with the WASM32 WASI build:

1. Check if your compiler supports WASI by running:

```bash
clang --target=wasm32-wasi -dM -E - < /dev/null | grep WASI
```

2. Ensure the WASI SDK is properly installed and available in your PATH or specified via the WASI_SDK_PATH environment variable.

3. Check the CMake output for any error messages related to the WASI build.

## Advanced Configuration

The WASM32 WASI build can be customized by modifying the following files:

-   `cmake/wasm32-wasi.cmake`: Toolchain file for WASM32 WASI
-   `cmake/CheckWASISupport.cmake`: Function to check if WASI support is available
-   `cmake/AddLibsndfileWasm32WASI.cmake`: Function to build libsndfile for WASM32 WASI
