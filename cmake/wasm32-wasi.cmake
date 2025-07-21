# Auto-generated WASI toolchain file
set(CMAKE_SYSTEM_NAME WASI)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR wasm32)
set(CMAKE_C_COMPILER_TARGET wasm32-wasi)
set(CMAKE_CXX_COMPILER_TARGET wasm32-wasi)

# Find clang that supports WASI
find_program(CMAKE_C_COMPILER NAMES clang-18 clang-17 clang-16 clang-15 clang-14 clang)
find_program(CMAKE_CXX_COMPILER NAMES clang++-18 clang++-17 clang++-16 clang++-15 clang++-14 clang++)

if(NOT CMAKE_C_COMPILER)
    message(FATAL_ERROR "Could not find clang compiler with WASI support")
endif()

# Set sysroot if WASI_SDK_PREFIX is defined
if(DEFINED ENV{WASI_SDK_PREFIX})
    set(CMAKE_SYSROOT "$ENV{WASI_SDK_PREFIX}/share/wasi-sysroot")
    set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})
endif()

# Add wasi-runtimes library path if WASI_RUNTIMES_LIB is defined
if(DEFINED WASI_RUNTIMES_LIB)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${WASI_RUNTIMES_LIB}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -L${WASI_RUNTIMES_LIB}")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -L${WASI_RUNTIMES_LIB}")
    # Also add to find root path for library discovery
    list(APPEND CMAKE_FIND_ROOT_PATH ${WASI_RUNTIMES_LIB})
endif()

# WASI doesn't support shared libraries
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")

# Don't look for programs in the host system
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# Only look for libraries and includes in the sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Disable features not available in WASI
set(CMAKE_SKIP_RPATH TRUE)
