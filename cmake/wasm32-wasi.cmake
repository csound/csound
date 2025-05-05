# Toolchain file for WASM32 WASI
set(CMAKE_SYSTEM_NAME WASI)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR wasm32)

# Set the cross compiler using WASI_SDK_PATH
if(DEFINED ENV{WASI_SDK_PATH})
    set(WASI_SDK_PATH $ENV{WASI_SDK_PATH})
    set(CMAKE_C_COMPILER ${WASI_SDK_PATH}/bin/clang)
    set(CMAKE_CXX_COMPILER ${WASI_SDK_PATH}/bin/clang++)
    set(CMAKE_AR ${WASI_SDK_PATH}/bin/llvm-ar)
    set(CMAKE_RANLIB ${WASI_SDK_PATH}/bin/llvm-ranlib)
    set(CMAKE_SYSROOT ${WASI_SDK_PATH}/share/wasi-sysroot)
else()
    message(FATAL_ERROR "WASI_SDK_PATH environment variable must be set to build for WASM32 WASI")
endif()

# Set the target
set(triple wasm32-wasi)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER_TARGET ${triple})

# Don't run the linker on compiler check
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Set compiler flags with sysroot
set(CMAKE_C_FLAGS_INIT "-fPIC -D__wasm__=1 --sysroot=${CMAKE_SYSROOT}")
set(CMAKE_CXX_FLAGS_INIT "-fPIC -D__wasm__=1 -fno-exceptions --sysroot=${CMAKE_SYSROOT}")

# Static linking only
set(BUILD_SHARED_LIBS OFF)