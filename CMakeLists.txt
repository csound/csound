if (APPLE)
    cmake_minimum_required(VERSION 3.28)
else()
    cmake_minimum_required(VERSION 3.13.4)
endif()

set (CMAKE_CXX_STANDARD 11)

cmake_policy(SET CMP0077 NEW)
cmake_policy(SET CMP0075 NEW)
cmake_policy(SET CMP0091 NEW)

set(CSOUND_ESP32_TARGET "" CACHE STRING "ESP32 target chip (esp32p4 or esp32s3)")
set_property(CACHE CSOUND_ESP32_TARGET PROPERTY STRINGS "" esp32p4 esp32s3)
if(CSOUND_ESP32_TARGET)
    if(NOT CSOUND_ESP32_TARGET MATCHES "^(esp32p4|esp32s3)$")
        message(FATAL_ERROR "CSOUND_ESP32_TARGET must be esp32p4 or esp32s3")
    endif()
    if(EXISTS "${CMAKE_BINARY_DIR}/CMakeCache.txt" AND
       NOT DEFINED CSOUND_CONFIGURED_ESP32_TARGET)
        message(FATAL_ERROR "Use a fresh build directory for an ESP32 target")
    endif()
    if(DEFINED CSOUND_CONFIGURED_ESP32_TARGET AND
       NOT CSOUND_CONFIGURED_ESP32_TARGET STREQUAL CSOUND_ESP32_TARGET)
        message(FATAL_ERROR "Use a fresh build directory when changing the ESP32 target")
    endif()
    if(USE_VCPKG)
        message(FATAL_ERROR "The ESP32 build does not use Vcpkg")
    endif()
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/platform/esp32/toolchain.cmake"
        CACHE FILEPATH "CMake toolchain file")
    include(platform/esp32/defaults.cmake)
    set(CSOUND_CONFIGURED_ESP32_TARGET "${CSOUND_ESP32_TARGET}" CACHE INTERNAL "Configured ESP32 chip")
elseif(CSOUND_CONFIGURED_ESP32_TARGET)
    message(FATAL_ERROR "Use a fresh build directory when leaving the ESP32 target")
endif()

include(cmake/cmake-utilities.cmake)

# silence RPATH cmake warning
set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 12)
endif()

option(USE_VCPKG "Use VCPKG to download and manage dependencies" OFF)

if (USE_VCPKG)
    # Detect correct triplet based on CMake env
    if (ANDROID)
        if(CMAKE_TARGET_ARCHITECTURE STREQUAL "arm")
            set(VCPKG_TARGET_TRIPLET "arm-android")
            set(CMAKE_ANDROID_ARCH_ABI armeabi-v7a)
        elseif(CMAKE_TARGET_ARCHITECTURE STREQUAL "arm64")
            set(VCPKG_TARGET_TRIPLET "arm64-android")
            set(CMAKE_ANDROID_ARCH_ABI arm64-v8a)
        elseif(CMAKE_TARGET_ARCHITECTURE STREQUAL "x86")
            set(VCPKG_TARGET_TRIPLET "x86-android")
            set(CMAKE_ANDROID_ARCH_ABI x86)
        elseif(CMAKE_TARGET_ARCHITECTURE STREQUAL "x64")
            set(VCPKG_TARGET_TRIPLET "x64-android")
            set(CMAKE_ANDROID_ARCH_ABI x86_64)
        endif()
        set(CMAKE_SYSTEM_VERSION 21)
        set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/platform/androidcross/android.cmake)
    elseif (OSXCROSS_TARGET)
        if (IOS)
            if(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
                set(VCPKG_TARGET_TRIPLET "x64-ioscross")
                set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/platform/ioscross/ios-x64.cmake)
            elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
                set(VCPKG_TARGET_TRIPLET "arm64-ioscross")
                set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/platform/ioscross/ios-arm64.cmake)
            endif()
        else()
            if(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
                set(VCPKG_TARGET_TRIPLET "x64-osxcross")
                set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/platform/osxcross/osx-x64.cmake)
            elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
                set(VCPKG_TARGET_TRIPLET "arm64-osxcross")
                set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/platform/osxcross/osx-arm64.cmake)
            endif()
        endif()
    elseif (CMAKE_SYSTEM_NAME STREQUAL "MinGW" AND UNIX)
        # Detect 32-bit vs 64-bit MinGW based on compiler
        if (CMAKE_C_COMPILER MATCHES "i686-w64-mingw32")
            set(VCPKG_TARGET_TRIPLET "x86-mingw-static")
            set(VCPKG_TARGET_ARCHITECTURE x86)
        else()
            set(VCPKG_TARGET_TRIPLET "x64-mingw-static")
            set(VCPKG_TARGET_ARCHITECTURE x64)
        endif()
        set(VCPKG_INSTALL_OPTIONS "--allow-unsupported")
        set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/toolchains/mingw.cmake)
        set(VCPKG_CMAKE_SYSTEM_NAME MinGW)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
        set(VCPKG_TARGET_TRIPLET "wasm32-emscripten")
        set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/platform/wasm-emscripten/wasm32.cmake)
        set(VCPKG_CMAKE_SYSTEM_NAME Emscripten)
    elseif (WIN32)
        # Match dependencies to the Visual Studio target architecture
        if (CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
            set(VCPKG_TARGET_TRIPLET "x86-windows-static")
        elseif (CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
            set(VCPKG_TARGET_TRIPLET "arm64-windows-static")
        else()
            set(VCPKG_TARGET_TRIPLET "x64-windows-static")
        endif()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    elseif(APPLE)
        execute_process(COMMAND uname -m OUTPUT_VARIABLE ARCH)
        if (ARCH STREQUAL "arm64\n")
            set(VCPKG_TARGET_TRIPLET "arm64-osx")
        else()
            set(VCPKG_TARGET_TRIPLET "x64-osx")
        endif()
    elseif (UNIX)
        set(VCPKG_TARGET_TRIPLET "x64-linux")
    endif()

    set(VCPKG_FEATURE_FLAGS versions)
    set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON)
    set(VCPKG_OVERLAY_TRIPLETS ${CMAKE_CURRENT_SOURCE_DIR}/platform/vcpkg/triplets)
    set(VCPKG_OVERLAY_PORTS ${CMAKE_CURRENT_SOURCE_DIR}/platform/vcpkg/ports)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
        CACHE STRING "Vcpkg toolchain file")
endif()

message(STATUS "VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET}")
message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")

project(Csound)

enable_testing()

string(TOUPPER "${CMAKE_SYSTEM_NAME}" CMAKE_SYSTEM_NAME_UPPER)

# Collect private libraries for static linking (for pkg-config --static)
set(CSOUND_PRIVATE_LIBS "")

set(libcsound_private_include_dirs "")
set(libcsound_public_include_dirs "")

SET(BUILD_SHARED_LIBS ON)

if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  set(CMAKE_COMPILER_IS_CLANG 1)
endif()

message(STATUS "Csound directory: ${CMAKE_HOME_DIRECTORY}")


# Project definitions
set(CS_VERSION "7")
set(CS_SUBVER "0")
set(CS_PATCHLEVEL "0")
set(APIVERSION "${CS_VERSION}.${CS_SUBVER}")

message(STATUS "Csound version: ${CS_VERSION}.${CS_SUBVER}.${CS_PATCHLEVEL}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/README.md.in ${CMAKE_CURRENT_SOURCE_DIR}/README.md)

set(MACOSX_FRAMEWORK_BUNDLE_VERSION "${CS_VERSION}.${CS_SUBVER}.${CS_PATCHLEVEL}")
set(MACOSX_FRAMEWORK_SHORT_VERSION_STRING "${CS_VERSION}.${CS_SUBVER}.${CS_PATCHLEVEL}")
set(MACOSX_FRAMEWORK_IDENTIFIER "CSND")

# Relative install paths
set(EXECUTABLE_INSTALL_DIR "bin")
set(LOCALE_INSTALL_DIR "share/locale")
set(HEADER_INSTALL_DIR "include/csound")
set(SAMPLES_INSTALL_DIR "/samples")

if(OSXCROSS_TARGET)
    set(CS_FRAMEWORK_DEST "${CMAKE_INSTALL_PREFIX}/Library/Frameworks" CACHE PATH "Csound framework path")
else()
    set(CS_FRAMEWORK_DEST "$ENV{HOME}/Library/Frameworks" CACHE PATH "Csound framework path")
endif()

set(CS_OPCODE_DIR "" CACHE PATH "opcodedir path")


if(APPLE)
    set(CMAKE_INSTALL_RPATH ${CS_FRAMEWORK_DEST})
endif()

include(TestBigEndian)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckLibraryExists)
include(CMakeParseArguments)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules/)
list(APPEND CMAKE_PREFIX_PATH /usr /usr/local /opt/local /sw)
message(STATUS "CMake module path: ${CMAKE_MODULE_PATH}")

if (CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    execute_process(
        COMMAND brew --prefix bison
        RESULT_VARIABLE BREW_BISON
        OUTPUT_VARIABLE BREW_BISON_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (BREW_BISON EQUAL 0 AND EXISTS "${BREW_BISON_PREFIX}")
        message(STATUS "Found Bison keg installed by Homebrew at ${BREW_BISON_PREFIX}")
        set(BISON_EXECUTABLE "${BREW_BISON_PREFIX}/bin/bison")
    elseif(EXISTS "/usr/local/bin/bison")
        message(STATUS "Bison in /usr/local/bin")
        set(BISON_EXECUTABLE "/usr/local/bin/bison")
    endif()

    execute_process(
        COMMAND brew --prefix flex
        RESULT_VARIABLE BREW_FLEX
        OUTPUT_VARIABLE BREW_FLEX_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (BREW_FLEX EQUAL 0 AND EXISTS "${BREW_FLEX_PREFIX}")
        message(STATUS "Found Flex keg installed by Homebrew at ${BREW_FLEX_PREFIX}")
        set(FLEX_EXECUTABLE "${BREW_FLEX_PREFIX}/bin/flex")
   elseif(EXISTS "/usr/local/bin/flex")
        message(STATUS "Flex in /usr/local/bin")
        set(FLEX_EXECUTABLE "/usr/local/bin/flex")
    endif()

endif()

### COMPILER OPTIMIZATION FLAGS
option(USE_COMPILER_OPTIMIZATIONS "Use the default Csound compiler optimization flags" ON)
option(USE_AVX2 "Require AVX2 instructions on x86-64 (no runtime CPU dispatch)" OFF)
option(USE_APPLE_M1_BASELINE "On macOS arm64, tune for Apple M1 as the baseline CPU (-mcpu=apple-m1)" ON)
## USER OPTIONS ##
# Optional targets, they should all default to ON (check_deps will disable them if not possible to build)
option(USE_DOUBLE "Set to use double-precision floating point for audio samples." ON)
option(USE_FLOAT "Use single precision for cs_float and cs_double." OFF)
if(USE_DOUBLE AND USE_FLOAT)
    message(FATAL_ERROR "USE_FLOAT requires -DUSE_DOUBLE=OFF")
endif()
option(USE_LIBSNDFILE "Use libsndfile for sndfile IO, if available" ON)
option(BUILD_PERFTHREAD_CLASS "Include the Csound performance thread support (C++) " ON)
option(BUILD_UTILITIES "Build stand-alone executables for utilities that can also be used with -U" ON)
option(NEW_PARSER_DEBUG "Enable tracing of new parser" OFF)
option(BUILD_MULTI_CORE "Enable building for multicore system" ON)
option(FAIL_MISSING "Fail when a required external dependency is not present (useful for packagers)" OFF)
option(USE_GETTEXT "Use the Gettext internationalization library" ON)
option(BUILD_STATIC_LIBRARY "Also build a static version of the csound library" OFF)
option(USE_LRINT "Use lrint/lrintf for converting floating point values to integers." ON)
option(USE_CURL "Use CURL library" ON)
option(USE_WASMTIME "Load WebAssembly opcode libraries with Wasmtime when available" ON)
option(USE_DEFAULT_OPCODEDIR "Use default opcodedir for the system" ON)
option(BUILD_INSTALLER "Build installer" OFF)
option(BUILD_TESTS "Build tests" OFF)
option(LEXER_DEBUG "Build lexer with debug option" OFF)
option(USE_GIT_COMMIT "Show the git commit in version information" ON)
option(REQUIRE_PTHREADS "For non-Windows systems, set whether Csound will use threads or not" ON)
option(USE_ATOMIC_BUILTIN "Use Atomic Builtins if supported" ON)
option(BUILD_DOCS "Build documentation (requires doxygen)" OFF)
option(DEBUG_ERROR_ON_WARNING "For a debug build, error on a compiler# warning" OFF)
option(BARE_METAL "To configure for bare metal build" OFF)
option(CUSTOM_MALLOC "Use custom malloc in bare metal builds" OFF)
option(BUILD_PLUGINS "Build external opcodes as plugins" OFF)
if(CMAKE_SYSTEM_NAME_UPPER STREQUAL "WASI")
    # Engine, browser host, and side modules share this memory ABI.
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/platform/wasm-wasi/src/shared-memory-max.txt"
         CSOUND_WASI_FIXED_MAX_MEMORY)
    if(DEFINED CSOUND_WASI_MAX_MEMORY AND
       NOT CSOUND_WASI_MAX_MEMORY STREQUAL CSOUND_WASI_FIXED_MAX_MEMORY)
        message(FATAL_ERROR
            "Set the shared memory limit in platform/wasm-wasi/src/shared-memory-max.txt and rebuild the engine, browser bundle, and plugins together")
    endif()
    set(CSOUND_WASI_MAX_MEMORY "${CSOUND_WASI_FIXED_MAX_MEMORY}")
    set(CSOUND_WASI_LINK_OPTIONS
        "-pthread;-Wl,--import-memory,--export-memory,--max-memory=${CSOUND_WASI_MAX_MEMORY}")
    set(BUILD_WASI_REALTIME "AUTO" CACHE STRING
        "WASI pthread support: AUTO, ON, or OFF")
    set_property(CACHE BUILD_WASI_REALTIME PROPERTY STRINGS AUTO ON OFF)
    string(TOUPPER "${BUILD_WASI_REALTIME}" WASI_REALTIME_REQUESTED)
    if(NOT WASI_REALTIME_REQUESTED MATCHES "^(AUTO|ON|OFF)$")
        message(FATAL_ERROR "BUILD_WASI_REALTIME must be AUTO, ON, or OFF")
    endif()
    set(BUILD_WASI_REALTIME OFF)
    if(NOT WASI_REALTIME_REQUESTED STREQUAL "OFF" AND REQUIRE_PTHREADS
       AND NOT BUILD_PLUGINS
       AND USE_ATOMIC_BUILTIN)
        set(THREADS_PREFER_PTHREAD_FLAG ON)
        find_package(Threads)
        include(cmake/CheckWasiRealtime.cmake)
        if(HAVE_WASI_REALTIME_RUNTIME AND Threads_FOUND AND CMAKE_USE_PTHREADS_INIT)
            set(BUILD_WASI_REALTIME ON)
        endif()
    endif()
    if(WASI_REALTIME_REQUESTED STREQUAL "ON" AND NOT BUILD_WASI_REALTIME)
        message(FATAL_ERROR
            "WASI realtime requires a threaded libc, lock-free atomics, and statically linked opcodes")
    endif()
    message(STATUS "WASI pthread support: ${BUILD_WASI_REALTIME}")
    option(BUILD_WASI_REALTIME_TESTS
           "Include private browser realtime test probes" OFF)
    option(BUILD_WASI_BROWSER
           "Build WASI for a browser host instead of as a standalone command"
           OFF)
else()
    if(BUILD_WASI_REALTIME STREQUAL "ON")
        message(FATAL_ERROR "BUILD_WASI_REALTIME requires a WASI toolchain")
    endif()
    set(BUILD_WASI_REALTIME OFF)
    # Keep this WASI-only mode out of native platform configurations, including
    # caches left behind by a previous cross-compilation.
    unset(BUILD_WASI_BROWSER CACHE)
    set(BUILD_WASI_BROWSER OFF)
endif()
if(BUILD_WASI_REALTIME)
    if(BUILD_PLUGINS OR NOT REQUIRE_PTHREADS)
        message(FATAL_ERROR
            "WASI realtime requires pthreads and statically linked opcodes")
    endif()
    # The WASI host uses one render caller and the existing init worker.
    set(BUILD_MULTI_CORE OFF)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -DCSOUND_WASI_REALTIME")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -DCSOUND_WASI_REALTIME")
    string(REPLACE ";" " " wasi_link_flags "${CSOUND_WASI_LINK_OPTIONS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${wasi_link_flags}")
endif()
option(BUILD_BELA "Build Bela main program" OFF)
option(BUILD_WITH_LTO "Build with link-time optimisations" OFF)
option(USE_SHORT_TABLE_LENGTH  "Build with original max table length 2^24" OFF)
option(USE_ASA  "Use address sanitizer in Debug build" ON)
option(USE_STATIC_DEPS "Use static dependencies for libsndfile" OFF)
option(USE_LIBSAMPLERATE "Use libsamplerate for sample rate conversion" ON)
option(USE_LOCK_BARRIER "Use lock-based barrier in PARCS " OFF)
option(ALIGN_MEMORY "Align memory allocation to cs_float size" OFF)
option(PRESERVE_NAN "Force preserving NaNs against optimisation removal" ON)
# Build metadata overrides for reproducible/sandboxed builds (e.g. Nix).
set(CSOUND_GIT_HASH "" CACHE STRING "Override git commit shown in version output")
set(CSOUND_BUILD_DATE "" CACHE STRING "Override build date shown in version output (e.g. 2026-02-12)")

set(HAVE_WASMTIME OFF)
if(USE_WASMTIME)
    # Probe the needed API instead of guessing support from the Csound target.
    find_package(Wasmtime 47 QUIET MODULE)
    if(Wasmtime_FOUND)
        set(HAVE_WASMTIME ON)
        message(STATUS "Using Wasmtime ${Wasmtime_VERSION} for WebAssembly opcode libraries")
    else()
        message(STATUS "Compatible Wasmtime C API with compiler and cache support not found; WebAssembly opcode libraries are disabled")
    endif()
else()
    message(STATUS "Wasmtime opcode support is disabled by USE_WASMTIME=OFF")
endif()


# secret rabbit code
if (USE_LIBSAMPLERATE)
    if (USE_VCPKG)
      find_package(SampleRate CONFIG REQUIRED)
      assign_bool(SampleRate_FOUND TARGET SampleRate::samplerate)
    else()
       find_package(SampleRate MODULE)
    endif()
endif()

if(SampleRate_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_SRC")
    message(STATUS "Using Secret Rabbit Code")
endif()


if(USE_SHORT_TABLE_LENGTH)
     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSHORT_TABLE_LENGTH")
    message(STATUS "Using short max table length")
endif()


if(ALIGN_MEMORY)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DALIGN_MEMORY")
    message(STATUS "Aligning memory to cs_float size")
else()
    message(STATUS "No memory alignment")
endif()


# Include this after the install path definitions so we can override them here.
# Also after function definitions so we can use them there
# Also after user options so that the default values don't overwrite the custom files values
find_file(CUSTOM_CMAKE "Custom.cmake" HINTS ${CMAKE_HOME_DIRECTORY})

if(CUSTOM_CMAKE)
    message(STATUS "Including Custom.cmake file: ${CUSTOM_CMAKE}")
    include(${CUSTOM_CMAKE})
else()
    message(STATUS "Not using Custom.cmake file.")
endif()


if(BUILD_WITH_LTO)
set(CSOUNDLIB_SHARED_LINK_OPTIONS -flto -Werror=odr
-Werror=lto-type-mismatch -Werror=strict-aliasing)
message(STATUS "Building with link-time optimisations")
else()
set(CSOUNDLIB_SHARED_LINK_OPTIONS "")
message(STATUS "Not building with link-time optimisations")
endif()

if(BARE_METAL)
message(STATUS "Building for bare metal")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -DBARE_METAL")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBARE_METAL")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -DNO_CSOUND_SERVER")

    if(CUSTOM_MALLOC)
     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -DCUSTOM_MALLOC")
    # the default is for STM32H7: -DMALLOC_BASE=0xC0000000
    endif()
endif()

# in Release configuration, set NDEBUG
if(${CMAKE_BUILD_TYPE} MATCHES "Release")
    message(STATUS "Building for release")
    add_definitions("-DNDEBUG")
elseif(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    message(STATUS "Building for debug")
    add_definitions("-DBETA")
endif()

# set -Werror if in Debug configuration
if(NOT MSVC AND NOT WASM)
#ifdef BARE_METAL
    set(CMAKE_CXX_FLAGS_RELEASE "-Os")
    set(CMAKE_C_FLAGS_RELEASE "-Os")
#else
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    set(CMAKE_C_FLAGS_RELEASE "-O3")
#endif
    if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
       set(USE_COMPILER_OPTIMIZATIONS OFF)
        if (DEBUG_ERROR_ON_WARNING)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
        endif()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
        if(APPLE)
            if(OSXCROSS_TARGET)
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshorten-64-to-32 -Wpointer-sign")
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wshorten-64-to-32 -Wpointer-sign")
            else()
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshorten-64-to-32 -Wpointer-sign")
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wshorten-64-to-32 -Wpointer-sign")
               if(USE_ASA)
                message(STATUS  "using address sanitizer")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wshorten-64-to-32 -fsanitize=address")
                else()
                message(STATUS  "not using address sanitizer")
               endif()
            endif()
        endif()
    endif()
endif()

if(USE_COMPILER_OPTIMIZATIONS)
    include(cmake/CompilerOptimizations.cmake)
    message(STATUS  "USING COMPILER OPTIMISATIONS")
else()
    message(STATUS  "NOT USING COMPILER OPTIMISATIONS")
endif()

# AVX2 is opt-in. Compiler support does not establish that the user's CPU
# supports these instructions, and global flags provide no runtime fallback.
if(USE_AVX2)
    # Restrict to x86-64; CMAKE_SYSTEM_PROCESSOR may be "x86_64" or "AMD64"
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        if(MSVC)
            check_cxx_compiler_flag("/arch:AVX2" COMPILER_SUPPORTS_AVX2)
            if(COMPILER_SUPPORTS_AVX2)
                set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   /arch:AVX2")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
                message(STATUS "AVX2 enabled (/arch:AVX2): binaries require an AVX2-capable CPU and OS")
            else()
                message(STATUS "AVX2 requested but compiler does not support /arch:AVX2 – skipping")
            endif()
        else()
            check_c_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
            if(COMPILER_SUPPORTS_AVX2)
                set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -mavx2")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
                message(STATUS "AVX2 enabled (-mavx2): binaries require an AVX2-capable CPU and OS")
            else()
                message(STATUS "AVX2 requested but compiler does not support -mavx2 – skipping")
            endif()
        endif()
    else()
        message(STATUS "USE_AVX2 is ON but target is not x86-64 (${CMAKE_SYSTEM_PROCESSOR}) – skipping")
    endif()
else()
    message(STATUS "AVX2 support disabled")
endif()

# Apple M1 baseline tuning – macOS arm64 only, optional, enabled by default
if(USE_APPLE_M1_BASELINE)
    if(APPLE AND NOT IOS AND NOT OSXCROSS_TARGET
       AND (CMAKE_OSX_ARCHITECTURES STREQUAL "arm64"
            OR (NOT CMAKE_OSX_ARCHITECTURES AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")))
        check_c_compiler_flag("-mcpu=apple-m1" COMPILER_SUPPORTS_APPLE_M1)
        if(COMPILER_SUPPORTS_APPLE_M1)
            set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -mcpu=apple-m1")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=apple-m1")
            message(STATUS "Apple M1 baseline tuning enabled (-mcpu=apple-m1)")
        else()
            message(STATUS "USE_APPLE_M1_BASELINE is ON but compiler does not support -mcpu=apple-m1 – skipping")
        endif()
    else()
        message(STATUS "USE_APPLE_M1_BASELINE is ON but not building for macOS arm64 – skipping")
    endif()
else()
    message(STATUS "Apple M1 baseline tuning disabled")
endif()

if(APPLE)
    #list(APPEND CMAKE_REQUIRED_INCLUDES "time.h")
    CHECK_FUNCTION_EXISTS(clock_gettime CLOCK_GETTIME)
    if(${CLOCK_GETTIME})
        message(STATUS  "clock_gettime() found")
        set(CMAKE_C_FLAGS "-DHAVE_CLOCK_GETTIME ${CMAKE_C_FLAGS}")
    else()
        message(STATUS  "clock_gettime() not found")
    endif()
endif()

if(OSXCROSS_TARGET)
    set(CMAKE_C_FLAGS "-I${OSXCROSS_SDK}/usr/include ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "-I${OSXCROSS_SDK}/usr/include ${CMAKE_CXX_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -L${CMAKE_OSX_SYSROOT}/usr/lib")
endif()

# library installation directories
option(USE_LIB64 "Set to on to set installation directory for libraries to lib64" OFF)
if(USE_LIB64)
    if(WIN32)
        set(LIBRARY_INSTALL_DIR "lib64")
    else()
        set(LIBRARY_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib64")
    endif()
    add_definitions("-DLIB64")
elseif(NOT DEFINED LIBRARY_INSTALL_DIR)
    if(WIN32)
        set(LIBRARY_INSTALL_DIR "lib")
    else()
        set(LIBRARY_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib")
    endif()
endif()
message(STATUS "LIBRARY INSTALL DIR: ${LIBRARY_INSTALL_DIR}")

if(WIN32)
    # Keep the Inno Setup library destination in sync with the exported targets.
    string(REPLACE "/" "\\" CSOUND_INSTALLER_LIBRARY_DIR "${LIBRARY_INSTALL_DIR}")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/installer/windows/csound_library_dir.iss.in"
        "${CMAKE_CURRENT_BINARY_DIR}/csound_library_dir.iss"
        @ONLY)
endif()

if(USE_DOUBLE)
    message(STATUS "Building with 64-bit floats")
    set(PLUGIN_INSTALL_DIR "${LIBRARY_INSTALL_DIR}/csound/plugins64-${APIVERSION}")
else()
    message(STATUS "Building with 32-bit floats")
    set(PLUGIN_INSTALL_DIR "${LIBRARY_INSTALL_DIR}/csound/plugins-${APIVERSION}")
endif()

if(WIN32 AND NOT MSVC)
    if(EXISTS "C:/MinGW/include")
        list(APPEND libcsound_private_include_dirs "C:/MinGW/include")
    else()
        MESSAGE(STATUS "MinGW include dir not found.")
    endif()
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--add-stdcall-alias")
endif()




if(WIN32)
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
    set(CMAKE_SHARED_MODULE_PREFIX "")

    set(CSOUND_WINDOWS_LIBRARIES kernel32 ws2_32)

    if(MSVC)
        # Experimented with flags but did not make change performance results
        #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Oi /fp:fast /arch:AVX2")
        #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Oi /fp:fast /arch:AVX2")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mstackrealign -static-libgcc -static")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mstackrealign -static-libstdc++ -static-libgcc -static")
        set_target_properties(${CSOUNDLIB} PROPERTIES LINK_FLAGS "-static-libstdc++ -static-libgcc -static")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libstdc++ -static-libgcc -static")
    endif()
endif()


set(BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

check_c_compiler_flag(-fvisibility=hidden HAS_VISIBILITY_HIDDEN)
check_cxx_compiler_flag(-fvisibility=hidden HAS_CXX_VISIBILITY_HIDDEN)
if (HAS_VISIBILITY_HIDDEN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
endif()
if (HAS_CXX_VISIBILITY_HIDDEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden ")
endif()

# Keep NaN and infinity checks valid even when -ffast-math is enabled.
# With GCC, this override also undefines __FAST_MATH__, which hides glibc's
# automatic vector-math declarations. Do not restore finite-only assumptions
# or define that macro by hand to recover SIMD calls. An explicit vector
# library can support nonfinite inputs, but needs accuracy, platform support
# and Csound workload checks. See the investigation in #3491.
if (PRESERVE_NAN)
    message(STATUS "Preserving NaNs")
    check_c_compiler_flag(-fno-finite-math-only HAS_NO_FINITE_MATH_ONLY)
    check_cxx_compiler_flag(-fno-finite-math-only HAS_CXX_NO_FINITE_MATH_ONLY)
    if (HAS_NO_FINITE_MATH_ONLY)
        add_compile_options("$<$<COMPILE_LANGUAGE:C>:-fno-finite-math-only>")
    endif()
    if (HAS_CXX_NO_FINITE_MATH_ONLY)
        add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-fno-finite-math-only>")
    endif()
endif()

# Probe the same flags used to compile sources, without passing compile-only
# flags to the linker. Function scope keeps these probe settings local.
function(csound_check_nonfinite_math)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
    if(PRESERVE_NAN AND HAS_NO_FINITE_MATH_ONLY)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-finite-math-only")
    endif()
    if(PRESERVE_NAN AND HAS_CXX_NO_FINITE_MATH_ONLY)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-finite-math-only")
    endif()

    check_c_source_compiles("
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
#error __FINITE_MATH_ONLY__ is enabled
#endif
int main(void) { return 0; }
" CSOUND_C_PRESERVES_NONFINITE)
    if (NOT CSOUND_C_PRESERVES_NONFINITE)
        message(FATAL_ERROR "C compiler has finite-math-only enabled; Csound requires NaN and Inf checks")
    endif()

    check_cxx_source_compiles("
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
#error __FINITE_MATH_ONLY__ is enabled
#endif
int main() { return 0; }
" CSOUND_CXX_PRESERVES_NONFINITE)
    if (NOT CSOUND_CXX_PRESERVES_NONFINITE)
        message(FATAL_ERROR "C++ compiler has finite-math-only enabled; Csound requires NaN and Inf checks")
    endif()
endfunction()
csound_check_nonfinite_math()

# C language standard.
# MSVC only gained /std:c11 in 19.27 and CMake only maps CMAKE_C_STANDARD to it
# from 3.18, but we are supporting cmake >= 3.13, so probe the switch directly
# there. Everywhere else use CMAKE_C_STANDARD, which CMake translates to the
# right compiler option for the toolchain (-std=gnu11 for GCC/Clang, -std=c11, /std:c11 for clang-cl,
#  and decays to gnu99 when C11 is unavailable.
if(MSVC)
    check_c_compiler_flag("/std:c11" HAS_C11)
    if(HAS_C11)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /std:c11")
    endif()
else()
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED OFF)
    set(CMAKE_C_EXTENSIONS ON)
endif()


if (HAS_CXX_VISIBILITY_HIDDEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
endif()

if(APPLE)
    if(IOS)
        message(STATUS "Building for iOS.")
    else()
        message(STATUS "Building for OSX")

        if(BUILD_INSTALLER)
            set(CS_FRAMEWORK_DEST "${CMAKE_INSTALL_PREFIX}")
        endif()

        set(DEFAULT_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk/)
        if(NOT CMAKE_OSX_SYSROOT AND EXISTS ${DEFAULT_OSX_SYSROOT})
            set(CMAKE_OSX_SYSROOT ${DEFAULT_OSX_SYSROOT})
        endif()

        message(STATUS "OSX: Setting Deployment Target to ${CMAKE_OSX_DEPLOYMENT_TARGET}")

        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_VECLIB")
        if(NOT VECLIB_PATH)
            set(VECLIB_PATH "/System/Library/Frameworks/")
        endif()
    endif()
endif()

# Set plugins install directory
if(USE_DOUBLE)
    if(APPLE)
        set(CSOUNDLIB "CsoundLib64")
        set(PLUGIN_INSTALL_DIR "${CS_FRAMEWORK_DEST}/${CSOUNDLIB}.framework/Versions/${APIVERSION}/Resources/Opcodes64")
    elseif(WIN32)
        set(CSOUNDLIB "csound${CS_VERSION}_64")
    else()
        set(CSOUNDLIB "csound64")
    endif()
else()
    if(APPLE)
        set(CSOUNDLIB "CsoundLib")
        set(PLUGIN_INSTALL_DIR "${CS_FRAMEWORK_DEST}/${CSOUNDLIB}.framework/Versions/${APIVERSION}/Resources/Opcodes")
    elseif(WIN32)
        set(CSOUNDLIB "csound${CS_VERSION}")
    else()
        set(CSOUNDLIB "csound")
    endif()
endif()

# set default user plugin directory (independent of CS_OPCODE_DIR)
if(APPLE)
    # dir relative to $HOME
    if(USE_DOUBLE)
      set(DEFAULT_USER_PLUGINDIR "Library/csound/${APIVERSION}/plugins64")
   else()
      set(DEFAULT_USER_PLUGINDIR "Library/csound/${APIVERSION}/plugins")
    endif()

    add_definitions("-DCS_DEFAULT_USER_PLUGINDIR=\"${DEFAULT_USER_PLUGINDIR}\"")
endif()

if(CS_OPCODE_DIR STREQUAL "")
if(APPLE)
    if(BUILD_INSTALLER)
       	set(CS_FRAMEWORK_FULL_PATH "/Library/Frameworks/${CSOUNDLIB}.framework/Versions/${APIVERSION}/Resources/Opcodes64")
    else()
        get_filename_component(CS_FRAMEWORK_FULL_PATH ${PLUGIN_INSTALL_DIR} ABSOLUTE)
    endif()

    add_definitions("-DCS_DEFAULT_PLUGINDIR=\"${CS_FRAMEWORK_FULL_PATH}\"")
    set(DEFAULT_OPCODEDIR "${CS_FRAMEWORK_FULL_PATH}")
endif()
else()
   add_definitions("-DCS_DEFAULT_PLUGINDIR=\"${CS_OPCODE_DIR}\"")
   set(DEFAULT_OPCODEDIR "${CS_OPCODE_DIR}")
endif()


# LINUX definition
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(LINUX YES)
    message(STATUS "LINUX operating system found")
else()
    set(LINUX NO)
endif()

if(LINUX)
    set(DEFAULT_OPCODEDIR "${PLUGIN_INSTALL_DIR}")
    add_definitions("-DCS_DEFAULT_PLUGINDIR=\"${DEFAULT_OPCODEDIR}\"")
    # dir relative to $HOME
    if(USE_DOUBLE)
        set(DEFAULT_USER_PLUGINDIR ".local/lib/csound/${APIVERSION}/plugins64")
    else()
        set(DEFAULT_USER_PLUGINDIR ".local/lib/csound/${APIVERSION}/plugins")
    endif()
    add_definitions("-DCS_DEFAULT_USER_PLUGINDIR=\"${DEFAULT_USER_PLUGINDIR}\"")
endif()

if(WIN32)
    # dir relaive to %LOCALAPPDATA%
    if(USE_DOUBLE)
        set(DEFAULT_USER_PLUGINDIR "csound/${APIVERSION}/plugins64")
    else()
        set(DEFAULT_USER_PLUGINDIR "csound/${APIVERSION}/plugin")
    endif()
    add_definitions("-DCS_DEFAULT_USER_PLUGINDIR=\"${DEFAULT_USER_PLUGINDIR}\"")
endif()

if(APPLE OR LINUX)
    message(STATUS "default plugin dir set to ${DEFAULT_OPCODEDIR}")
endif()

if(USE_DEFAULT_OPCODEDIR)
 add_definitions("-DUSE_DEFAULT_OPCODEDIR")
 message(STATUS "using default opcodedir")
endif()



set(BUILD_PLUGINS_DIR ${BUILD_DIR})
set(BUILD_BIN_DIR ${BUILD_DIR})
set(BUILD_LIB_DIR ${BUILD_DIR})

message(STATUS "BUILD_BIN_DIR set to ${BUILD_BIN_DIR}.")
message(STATUS "BUILD_LIB_DIR set to ${BUILD_LIB_DIR}.")
message(STATUS "BUILD_PLUGINS_DIR set to ${BUILD_PLUGINS_DIR}.")

# OS specific checks
TEST_BIG_ENDIAN(BIG_ENDIAN)

## HEADER/LIBRARY/OTHER CHECKS ##
# Check if libsndfile is available, if not, disable build
if(USE_LIBSNDFILE)
    if(USE_VCPKG)
        find_package(SndFile CONFIG REQUIRED)
        assign_bool(SndFile_FOUND TARGET SndFile::sndfile)
    else()
        message(STATUS "looking  at ${CMAKE_PREFIX_PATH}")
        find_package(SndFile MODULE)
        message(STATUS "libsndfile found: ${SndFile_LIBRARY}")
    endif()
endif()
check_deps(USE_LIBSNDFILE SndFile_FOUND)

# this makes sure make_plugin() can find headers
list(APPEND libcsound_private_include_dirs ${SndFile_INCLUDE_DIR})

# carry on setting up libsndifle if so
if(USE_LIBSNDFILE)
    add_compile_definitions("USE_LIBSNDFILE")
    set(CSOUND_PRIVATE_LIBS "${CSOUND_PRIVATE_LIBS} -lsndfile")

    if(NOT USE_VCPKG AND EMSCRIPTEN)
        list(APPEND libcsound_private_include_dirs ${Csound_SOURCE_DIR}/emscripten/deps/libsndfile-1.0.25/src/)
    endif()

    # add to required libraries so we can check for MIDI support
    list(APPEND CMAKE_REQUIRED_LIBRARIES SndFile::sndfile)

    check_c_source_compiles("#include <sndfile.h>
    int main(int argc, char** argv) {
    printf(\"SF_FORMAT_MPEG: %d\", SF_FORMAT_MPEG);
    return 0;
    }" HAVE_SF_FORMAT_MPEG)

    option(USE_MP3 "Use MP3 support in libsndfile" ON)
    check_deps(USE_MP3 HAVE_SF_FORMAT_MPEG)

    if (USE_MP3)
        message(STATUS  "Using libsndfile MP3 support")
        add_compile_definitions("SNDFILE_MP3")
    else()
        message(STATUS  "NOT Using libsndfile MP3 support")
    endif()
endif()

find_package(CURL)

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads)
# windows threads aren't supported
assign_bool(NOT_WINDOWS_THREADS NOT CMAKE_USE_WIN32_THREADS_INIT)
check_deps(REQUIRE_PTHREADS Threads_FOUND NOT_WINDOWS_THREADS)

if(REQUIRE_PTHREADS)
    list(APPEND CMAKE_REQUIRED_LIBRARIES Threads::Threads)
endif()

# Now, non required library searches

if(USE_ATOMIC_BUILTIN)
    include(cmake/CheckAtomic.cmake)
    if(HAVE_ATOMIC_BUILTIN)
        message(STATUS "Using atomic builtins.")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_ATOMIC_BUILTIN")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_ATOMIC_BUILTIN")
    else()
        message(STATUS "Not using atomic builtins - not found")
    endif()
else()
    message(STATUS "Not using atomic builtins - user disabled")
endif()

if(BUILD_WASI_REALTIME AND NOT REQUIRE_PTHREADS)
    message(FATAL_ERROR "WASI realtime requires CMake Threads support")
endif()
if(BUILD_WASI_REALTIME AND (NOT HAVE_ATOMIC_BUILTIN OR ATOMIC_LIB))
    message(FATAL_ERROR "WASI realtime requires lock-free atomic builtins")
endif()

find_package(Intl)
find_package(Gettext)
find_library(MATH_LIBRARY m)

if (NOT MATH_LIBRARY)
    set(MATH_LIBRARY "")
endif()

include(CheckSymbolExists)
cmake_push_check_state()
list(APPEND CMAKE_REQUIRED_LIBRARIES ${MATH_LIBRARY})
check_symbol_exists(lrint math.h FOUND_LRINT)
cmake_pop_check_state()

# MSVC / VCPKG won't find dirent.h by default. Need to add the path explicitly
find_file(DIRENT_H "dirent.h")

if(DIRENT_H)
    if(MSVC)
        get_filename_component(DIRENT_FOLDER ${DIRENT_H} DIRECTORY)
        list(APPEND CMAKE_REQUIRED_INCLUDES ${DIRENT_FOLDER})
    endif()
else()
    if(MSVC)
        message(FATAL_ERROR "Dirent.h not found, Csound will not be able to load dynamic plugins. Cannot continue with project generation")
    else()
     if(NOT BARE_METAL)
        message(WARNING "Dirent.h not found, Csound will not be able to load dynamic plugins such as real-time audio")
    endif()
    endif()
endif()

set(HEADERS_TO_CHECK
    unistd.h io.h fcntl.h stdint.h
    sys/time.h sys/types.h termios.h
    values.h winsock.h sys/socket.h
    dirent.h inttypes.h execinfo.h)

foreach(header ${HEADERS_TO_CHECK})
    # Convert to uppercase and replace [./] with _
    string(TOUPPER ${header} tmp)

    string(REGEX REPLACE [./] "_" upper_header ${tmp})
    check_include_file(${header} HAVE_${upper_header})
    if (HAVE_${upper_header})
        add_definitions("-DHAVE_${upper_header}")
    endif()
endforeach()

if (NOT HAVE_INTTYPES_H)
    message(FATAL_ERROR "Csound requires inttypes.h")
endif()

check_deps(USE_LRINT FOUND_LRINT)
if(USE_LRINT)
    add_definitions("-DUSE_LRINT")
endif()

## Check existence of CURL
if(USE_CURL)
    find_package(CURL)

    if(CURL_FOUND)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_CURL")
    else()
        message(STATUS "Not using CURL for urls - not found.")
    endif()
else()
    message(STATUS "Not using CURL for urls - disabled.")
endif()

# Flex/Bison for the new parser
if(WIN32)
    find_program(FLEX_EXECUTABLE NAMES flex win_flex win_flex.exe)
    find_program(BISON_EXECUTABLE NAMES bison win_bison win_bison.exe)
endif()

find_package(FLEX)
find_package(BISON)

if(NOT FLEX_EXECUTABLE)
    message(FATAL_ERROR "Csound requires the flex executable")
endif()

if(NOT BISON_EXECUTABLE)
    message(FATAL_ERROR "Csound requires the bison executable")
endif()

## MAIN TARGETS ##

set(libcsound_CFLAGS -D__BUILDING_LIBCSOUND)

if(APPLE)
    set(_csound_install_interface_include_dir
        "${CS_FRAMEWORK_DEST}/${CSOUNDLIB}.framework/Headers")
else()
    set(_csound_install_interface_include_dir "${HEADER_INSTALL_DIR}")
endif()

list(APPEND libcsound_public_include_dirs
    $<BUILD_INTERFACE:${Csound_SOURCE_DIR}>
    $<BUILD_INTERFACE:${Csound_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${Csound_SOURCE_DIR}/H>
    $<BUILD_INTERFACE:${Csound_SOURCE_DIR}/Engine>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${_csound_install_interface_include_dir}>
)

list(APPEND libcsound_private_include_dirs
    ${Csound_SOURCE_DIR}
    "${Csound_SOURCE_DIR}/util"
)

#checking pthread functions
if(REQUIRE_PTHREADS AND (NOT CMAKE_SYSTEM_NAME_UPPER STREQUAL "WASI" OR BUILD_WASI_REALTIME))
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_PTHREAD")

  if (NOT VCPKG_TARGET_TRIPLET STREQUAL "x64-mingw-static" AND
      NOT CMAKE_SYSTEM_NAME_UPPER STREQUAL "WASI")
    check_function_exists(pthread_spin_lock PTHREAD_SPIN_LOCK_EXISTS)
  endif()

  check_function_exists(pthread_barrier_init PTHREAD_BARRIER_INIT_EXISTS)

  if(PTHREAD_SPIN_LOCK_EXISTS AND NOT CMAKE_SYSTEM_NAME_UPPER STREQUAL "WASI")
     list(APPEND libcsound_CFLAGS -DHAVE_PTHREAD_SPIN_LOCK)
  endif()

  if(PTHREAD_BARRIER_INIT_EXISTS)
     list(APPEND libcsound_CFLAGS -DHAVE_PTHREAD_BARRIER_INIT)
  endif()
endif()

# get the git hash and pass it to csound
set(git_hash_values "none")

if(CSOUND_GIT_HASH)
    string(STRIP "${CSOUND_GIT_HASH}" git_hash_values)
    message(STATUS "HASH (override): ${git_hash_values}")
elseif(USE_GIT_COMMIT)
    find_package(Git)
    message(STATUS "GIT: ${GIT_EXECUTABLE}")

    if(GIT_FOUND)
        execute_process(COMMAND ${GIT_EXECUTABLE} "rev-parse" "HEAD"
                        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                        OUTPUT_VARIABLE git_hash_value)

        if (git_hash_value)
            string(STRIP ${git_hash_value} git_hash_values)
            message(STATUS "HASH: ${git_hash_values}")
        endif()
    endif()
endif()

set_source_files_properties(Top/csound.c PROPERTIES COMPILE_FLAGS -DGIT_HASH_VALUE=${git_hash_values})

# Provide a configurable date token used by include/version.h.in.
set(CS_PACKAGE_DATE_DEFINE "__DATE__")
if(CSOUND_BUILD_DATE)
    string(STRIP "${CSOUND_BUILD_DATE}" csound_build_date_value)
    set(CS_PACKAGE_DATE_DEFINE "\"${csound_build_date_value}\"")
    message(STATUS "DATE (override): ${csound_build_date_value}")
endif()

# The csound library
set(libcsound_SRCS
    OOps/json_ops.c
    third_party/yyjson/yyjson.c
    Top/csound.c
    Top/csound_perf.c
    Top/csound_events.c
    Top/csound_rtio.c
    Top/csound_messages.c
    Top/csound_time.c
    Top/argdecode.c
    Top/csound_debug.c
    Top/csmodule.c
    Top/getstring.c
    Top/main.c
    Top/new_opts.c
    Top/one_file.c
    Top/opcode.c
    Top/threads.c
    Top/utility.c
    Top/threadsafe.c
    Top/server.c
    Engine/auxfd.c
    Engine/cfgvar.c
    Engine/corfiles.c
    Engine/entry.c
    Engine/opcode_deprecation.c
    Engine/extract.c
    Engine/environ.c
    Engine/fgens.c
    Engine/filesys.c
    Engine/insert.c
    Engine/srconvert.c
    Engine/udo.c
    Engine/linevent.c
    Engine/memalloc.c
    Engine/memfiles.c
    Engine/musmon.c
    Engine/namedins.c
    Engine/rdscor.c
    Engine/pfield.c
    Engine/scsort.c
    Engine/scxtract.c
    Engine/sort.c
    Engine/sread.c
    Engine/swritestr.c
    Engine/twarp.c
    Engine/csound_type_system.c
    Engine/csound_standard_types.c
    Engine/csound_data_structures.c
    Engine/ugen.c
    InOut/soundfile.c
    InOut/libsnd.c
    InOut/libsnd_u.c
    InOut/midifile.c
    InOut/midirecv.c
    InOut/midisend.c
    InOut/winascii.c
    InOut/windin.c
    InOut/window.c
    InOut/winEPS.c
    InOut/circularbuffer.c
    OOps/array_ops.c
    OOps/aops.c
    OOps/audio_mul_add.c
    OOps/assert_ops.c
    OOps/bus.c
    OOps/cmath.c
    OOps/complex_ops.c
    OOps/diskin2.c
    OOps/disprep.c
    OOps/dumpf.c
    OOps/fftlib.c
    OOps/inst_ops.c
    OOps/lpred.c
    OOps/pffft.c
    OOps/pffft_double.c
    OOps/goto_ops.c
    OOps/midiinterop.c
    OOps/midiops.c
    OOps/midiops2.c
    OOps/midiops3.c
    OOps/midiout.c
    OOps/mxfft.c
    OOps/oscils.c
    OOps/pitch0.c
    OOps/pstream.c
    OOps/pvfileio.c
    OOps/pvsanal.c
    OOps/random.c
    OOps/readline.c
    OOps/remote.c
    OOps/schedule.c
    OOps/sndinfUG.c
    OOps/str_ops.c
    OOps/structs.c
    OOps/ugens1.c
    OOps/ugens2.c
    OOps/ugens3.c
    OOps/ugens4.c
    OOps/ugens5.c
    OOps/ugens6.c
    OOps/ugtabs.c
    OOps/ugrw1.c
    OOps/vdelay.c
    OOps/compile_ops.c
)

if(HAVE_WASMTIME)
    list(APPEND libcsound_SRCS Top/wasm_plugins.c)
endif()

if(CMAKE_SYSTEM_NAME_UPPER STREQUAL "WASI" AND BUILD_WASI_BROWSER)
    list(APPEND libcsound_SRCS Top/wasm_opcode_abi_check.c)
endif()

if(IOS AND NOT  OSXCROSS_TARGET)

set(libcsound_SRCS ${libcsound_SRCS}
  platform/ios/Csound-for-iOS/src/rtaunit.c
  platform/ios/Csound-for-iOS/src/iOSCsound.cpp
)
endif()

set(vbap_ops
    Opcodes/vbap.c
)

set(libcsound_static_SRCS  ${libcsound_SRCS})

set_source_files_properties(OOps/pffft.c OOps/pffft_double.c PROPERTIES
    INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/pffft/include/pffft")
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    # Suppress upstream SIMD selection messages without changing vendored files.
    set_property(SOURCE OOps/pffft.c OOps/pffft_double.c APPEND PROPERTY
        COMPILE_OPTIONS "-Wno-#pragma-messages")
endif()

# Keep JSON numbers precise even when DSP code uses fast-math. The parser is
# private to Csound, not a new exported library interface.
set_source_files_properties(OOps/json_ops.c third_party/yyjson/yyjson.c
    PROPERTIES COMPILE_DEFINITIONS
    "yyjson_api=;YYJSON_DISABLE_UTILS=1;YYJSON_DISABLE_INCR_READER=1")
if(MSVC)
    set_source_files_properties(OOps/json_ops.c third_party/yyjson/yyjson.c
        PROPERTIES COMPILE_OPTIONS "/fp:precise")
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(OOps/json_ops.c third_party/yyjson/yyjson.c
        PROPERTIES COMPILE_OPTIONS "-fno-fast-math")
endif()

if(WIN32 AND NOT MSVC)
    set_source_files_properties(Opcodes/sfont.c PROPERTIES
        COMPILE_FLAGS -mno-ms-bitfields)
endif()

if(NOT MSVC)
    set_source_files_properties(Opcodes/sfont.c PROPERTIES
        COMPILE_FLAGS -Wno-address-of-packed-member)
endif()

set(physmod_SRCS
    Opcodes/physutil.c
    Opcodes/modal4.c
    Opcodes/physmod.c
    Opcodes/mandolin.c
    Opcodes/singwave.c
    Opcodes/fm4op.c
    Opcodes/moog1.c
    Opcodes/shaker.c
    Opcodes/bowedbar.c
)

set(externs_SRCS
    Opcodes/babo.c
    Opcodes/bilbar.c
    Opcodes/compress.c
    Opcodes/eqfil.c
    Opcodes/Vosim.c
    Opcodes/squinewave.c
    Opcodes/pinker.c
    Opcodes/cpumeter.c
    Opcodes/sc_noise.c
    Opcodes/afilters.c
    Opcodes/wpfilters.c
    Opcodes/lufs.c
    Opcodes/date.c
    Opcodes/system_call.c
    Opcodes/serial.c
    Opcodes/counter.c
    Opcodes/liveconv.c
    Opcodes/sterrain.c
    Opcodes/gammatone.c
    Opcodes/sfont.c
    Opcodes/gendy.c
    Opcodes/phisem.c
    Opcodes/framebuffer.c
    Opcodes/cellular.c
    Opcodes/exciter.c
    Opcodes/buchla.c
    Opcodes/select.c
    Opcodes/platerev.c
    Opcodes/sequencer.c
    Opcodes/grain4.c
    Opcodes/loscilx.c
    Opcodes/minmax.c
    Opcodes/pan2.c
    Opcodes/vaops.c
    Opcodes/ugakbari.c
    Opcodes/harmon.c
    Opcodes/pitchtrack.c
    Opcodes/partikkel.c
    Opcodes/shape.c
    Opcodes/tabaudio.c
    Opcodes/tabsum.c
    Opcodes/crossfm.c
    Opcodes/pvlock.c
    Opcodes/fareyseq.c
    Opcodes/modmatrix.c
    Opcodes/scoreline.c
    Opcodes/ambicode1.c
    Opcodes/arrays.c
    Opcodes/emugens/emugens.c
    Opcodes/emugens/scugens.c
    Opcodes/zak.c
    Opcodes/paulstretch.c
)

if(CMAKE_SYSTEM_NAME_UPPER STREQUAL "WASI")
    if(NOT BUILD_WASI_REALTIME)
        set(HAVE_PTHREAD 0)
    endif()
    # Disable shared libraries for WASI to avoid -shared flag warning
    set(BUILD_SHARED_LIBS OFF)
    list(REMOVE_ITEM externs_SRCS Opcodes/serial.c)
    list(REMOVE_ITEM externs_SRCS Opcodes/system_call.c)
    if(BUILD_WASI_BROWSER)
        # Browser builds use JavaScript-hosted callbacks and imported memory.
        list(APPEND libcsound_CFLAGS -DCSOUND_WASI_BROWSER)
        list(APPEND libcsound_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/platform/wasm-wasi/src/csound_wasm.c)
        list(APPEND libcsound_static_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/platform/wasm-wasi/src/csound_wasm.c)
        list(APPEND libcsound_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/platform/wasm-wasi/src/realtime_api.c)
        list(APPEND libcsound_static_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/platform/wasm-wasi/src/realtime_api.c)
    else()
        # Standalone WASI has a conventional command entry point and no JS host.
        list(APPEND libcsound_CFLAGS -DCSOUND_WASI_CLI)
        # The deconv opcode and named GEN normally arrive through the stdutil
        # plugin. Link only their source into the no-dynamic-loader command.
        list(APPEND libcsound_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/util/mkir.c)
        list(APPEND libcsound_static_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/util/mkir.c)
    endif()
endif()

set(newgabops_SRCS
    Opcodes/gab/tabmorph.c
    Opcodes/gab/hvs.c
    Opcodes/gab/newgabopc.c
)

set(gens_SRCS
    Opcodes/ftest.c
    Opcodes/fareygen.c
    Opcodes/quadbezier.c
)

set(pitch_ops_SRCS
    Opcodes/pitch.c
    Opcodes/spectra.c
)

set(stdopcod_SRCS
    Opcodes/ambicode.c
    Opcodes/bbcut.c
    Opcodes/biquad.c
    Opcodes/butter.c
    Opcodes/clfilt.c
    Opcodes/cross2.c
    Opcodes/dam.c
    Opcodes/dcblockr.c
    Opcodes/filter.c
    Opcodes/flanger.c
    Opcodes/follow.c
    Opcodes/fout.c
    Opcodes/freeverb.c
    Opcodes/ftconv.c
    Opcodes/ftgen.c
    Opcodes/gab/gab.c
    Opcodes/gab/vectorial.c
    Opcodes/grain.c
    Opcodes/locsig.c
    Opcodes/lowpassr.c
    Opcodes/metro.c
    Opcodes/newfils.c
    Opcodes/nlfilt.c
    Opcodes/oscbnk.c
    Opcodes/pluck.c
    Opcodes/repluck.c
    Opcodes/reverbsc.c
    Opcodes/seqtime.c
    Opcodes/sndloop.c
    Opcodes/sndwarp.c
    Opcodes/space.c
    Opcodes/spat3d.c
    Opcodes/syncgrain.c
    Opcodes/ugens7.c
    Opcodes/ugens9.c
    Opcodes/ugensa.c
    Opcodes/uggab.c
    Opcodes/ugmoss.c
    Opcodes/ugnorman.c
    Opcodes/ugsc.c
    Opcodes/wave-terrain.c
    Opcodes/wterrain2.c
    Opcodes/stdopcod.c
    Opcodes/dbap.c
    Opcodes/savgol.c
    Opcodes/remap.c
    Opcodes/stm.c
    )

set(sock_ops_SRCS
    Opcodes/socksend.c
    Opcodes/sockrecv.c
)


set(hrtf_ops_SRCS
    Opcodes/hrtfopcodes.c
    Opcodes/hrtfearly.c
    Opcodes/hrtfreverb.c
    Opcodes/hrtferX.c
)

set(cs_pvs_ops_SRCS
    Opcodes/ifd.c
    Opcodes/partials.c
    Opcodes/psynth.c
    Opcodes/pvsbasic.c
    Opcodes/pvscent.c
    Opcodes/pvsdemix.c
    Opcodes/pvs_ops.c
    Opcodes/pvsband.c
    Opcodes/pvsbuffer.c
    Opcodes/pvsgendy.c
    )

set(oldpvoc_SRCS
    Opcodes/dsputil.c
    Opcodes/pvadd.c
    Opcodes/pvinterp.c
    Opcodes/pvocext.c
    Opcodes/pvread.c
    Opcodes/ugens8.c
    Opcodes/vpvoc.c
    Opcodes/pvoc.c
)

set(scanops_SRCS
    Opcodes/scansyn.c
    Opcodes/scansynx.c
)

set(unixops_SRCS
    Opcodes/control.c
    Opcodes/urandom.c
)

set(mp3in_SRCS
    Opcodes/mp3in.c
    InOut/libmpadec/layer1.c
    InOut/libmpadec/layer2.c
    InOut/libmpadec/layer3.c
    InOut/libmpadec/synth.c
    InOut/libmpadec/tables.c
    InOut/libmpadec/mpadec.c
    InOut/libmpadec/mp3dec.c)


if(NOT CMAKE_SYSTEM_NAME_UPPER STREQUAL "WASI")
    # Full set of C++ opcodes for non-WASI builds
    set(cppops_SRCS
       Opcodes/ampmidid.cpp
       Opcodes/mixer.cpp
       Opcodes/bformdec2.cpp
       Opcodes/doppler.cpp
       Opcodes/ftsamplebank.cpp
       Opcodes/padsynth_gen.cpp
       Opcodes/signalflowgraph.cpp
       Opcodes/arrayops.cpp
       Opcodes/lfsr.cpp
       Opcodes/pvsops.cpp
       Opcodes/trigEnvSegs.cpp
       Opcodes/tl/fractalnoise.cpp
    )
else()
    # Limited C++ opcodes for WASI builds
    set(cppops_SRCS
        Opcodes/arrayops.cpp
        Opcodes/bformdec2.cpp
        Opcodes/ftsamplebank.cpp
        Opcodes/padsynth_gen.cpp
        Opcodes/pvsops.cpp
        Opcodes/signalflowgraph.cpp
        Opcodes/tl/fractalnoise.cpp
        Opcodes/trigEnvSegs.cpp)
endif()

list(APPEND libcsound_SRCS)

if(NOT BUILD_PLUGINS)
 list(APPEND libcsound_SRCS ${cppops_SRCS} ${scanops_SRCS} ${physmod_SRCS}
 ${mp3in_SRCS} ${hrtf_ops_SRCS} ${sock_ops_SRCS} ${externs_SRCS}
 ${gens_SRCS} ${cs_pvs_ops_SRCS} ${newgabops_SRCS}  ${oldpvoc_SRCS}
 ${pitch_ops_SRCS} ${vbap_ops} ${stdopcod_SRCS}
)
 list(APPEND libcsound_CFLAGS -DCS_INTERNAL)
endif()

# The static library includes builtin opcodes even when the shared library
# loads them as plugins. Do not pass BUILD_PLUGINS to its sources or tests.
list(APPEND libcsound_static_SRCS ${stdopcod_SRCS} ${cs_pvs_ops_SRCS} ${physmod_SRCS}
${pitch_ops_SRCS}  ${oldpvoc_SRCS} ${scanops_SRCS} ${mp3in_SRCS} ${cppops_SRCS}
${hrtf_ops_SRCS} ${sock_ops_SRCS} ${externs_SRCS} ${gens_SRCS} ${pitch_ops_SRCS}
${newgabops_SRCS}  ${oldpvoc_SRCS} ${vbap_ops}
)



if(UNIX)
if(NOT BUILD_PLUGINS)
 list(APPEND libcsound_SRCS ${unixops_SRCS})
endif()
 list(APPEND libcsound_static_SRCS ${unixops_SRCS})
endif()

if(BUILD_PERFTHREAD_CLASS AND NOT CMAKE_SYSTEM_NAME_UPPER STREQUAL "WASI")
    list(APPEND libcsound_SRCS "Top/csPerfThread.cpp")
    list(APPEND libcsound_static_SRCS "Top/csPerfThread.cpp")
endif()

# Handling New Parser
set(YACC_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Engine/csound_orc.y)
set(YACC_OUT ${CMAKE_CURRENT_BINARY_DIR}/csound_orcparse.c)
set(YACC_OUTH ${CMAKE_CURRENT_BINARY_DIR}/csound_orcparse.h)

set(LEX_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Engine/csound_orc.lex)
set(LEX_OUT ${CMAKE_CURRENT_BINARY_DIR}/csound_orclex.c)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/csound_orclex.c
    PROPERTIES COMPILE_FLAGS "-Wno-implicit-fallthrough -Wno-misleading-indentation")

set(PRELEX_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Engine/csound_pre.lex)
set(PRELEX_OUT ${CMAKE_CURRENT_BINARY_DIR}/csound_prelex.c)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/csound_prelex.c
    PROPERTIES COMPILE_FLAGS "-Wno-implicit-fallthrough")

if(LEXER_DEBUG)
    message(STATUS "Building lexer with debug option")

    add_custom_command(
        OUTPUT ${LEX_OUT}
        DEPENDS ${LEX_SRC}
        COMMAND ${FLEX_EXECUTABLE} ARGS -d -B -t ${LEX_SRC} > ${LEX_OUT}
    )
else()
    add_custom_command(
        OUTPUT ${LEX_OUT}
        DEPENDS ${LEX_SRC}
        COMMAND ${FLEX_EXECUTABLE} ARGS -B -t  ${LEX_SRC} > ${LEX_OUT}
    )
endif()

add_custom_command(
    DEPENDS ${PRELEX_SRC}
    COMMAND ${FLEX_EXECUTABLE} ARGS -B ${PRELEX_SRC} > ${PRELEX_OUT}
    OUTPUT ${PRELEX_OUT}
)

add_custom_command(
    OUTPUT ${YACC_OUT} ${YACC_OUTH}
    DEPENDS ${YACC_SRC} ${LEX_OUT}
    COMMAND ${BISON_EXECUTABLE}
    ARGS -pcsound_orc -d --report=itemset -o ${YACC_OUT} ${YACC_SRC}
)

list(APPEND libcsound_SRCS
    ${LEX_OUT} ${YACC_OUT} ${PRELEX_OUT}
    Engine/csound_orc_semantics.c
    Engine/csound_orc_expressions.c
    Engine/csound_orc_optimize.c
    Engine/csound_orc_compile.c
    Engine/csound_orc_structs.c
    Engine/new_orc_parser.c
    Engine/symbtab.c)

list(APPEND libcsound_static_SRCS
    ${LEX_OUT} ${YACC_OUT} ${PRELEX_OUT}
    Engine/csound_orc_semantics.c
    Engine/csound_orc_expressions.c
    Engine/csound_orc_optimize.c
    Engine/csound_orc_compile.c
    Engine/csound_orc_structs.c
    Engine/new_orc_parser.c
    Engine/symbtab.c)


# The private audio multiply/add kernel must round twice, never contract to FMA.
# Scope these flags to the kernel, leaving the other arithmetic opcodes alone.
# Non-CMake builds use its volatile fallback instead.
if(MSVC)
    check_c_compiler_flag("/fp:strict" HAS_STRICT_AUDIO_MUL_ADD_FP)
    set(AUDIO_MUL_ADD_STRICT_FP_OPTIONS "/fp:strict")
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    check_c_compiler_flag("-fno-fast-math -ffp-contract=off" HAS_STRICT_AUDIO_MUL_ADD_FP)
    set(AUDIO_MUL_ADD_STRICT_FP_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(HAS_STRICT_AUDIO_MUL_ADD_FP)
    set_property(SOURCE OOps/audio_mul_add.c APPEND PROPERTY
        COMPILE_OPTIONS ${AUDIO_MUL_ADD_STRICT_FP_OPTIONS})
    set_property(SOURCE OOps/audio_mul_add.c APPEND PROPERTY
        COMPILE_DEFINITIONS CSOUND_FUSION_STRICT_FP=1)
endif()


set_source_files_properties(${YACC_OUT} GENERATED)
set_source_files_properties(${YACC_OUTH} GENERATED)
set_source_files_properties(${LEX_OUT} GENERATED)
set_source_files_properties(${PRELEX_OUT} GENERATED)

if(MSVC)
    set_source_files_properties(${LEX_OUT} PROPERTIES
        COMPILE_FLAGS -DYY_NO_UNISTD_H)
    set_source_files_properties(${PRELEX_OUT} PROPERTIES
        COMPILE_FLAGS -DYY_NO_UNISTD_H)
endif()

# Handling New Score Parser

##list(APPEND libcsound_CFLAGS -DSCORE_PARSER)

#set(YACC_SCOSRC ${CMAKE_CURRENT_SOURCE_DIR}/Engine/csound_sco.y)
#set(YACC_SCOOUT ${CMAKE_CURRENT_BINARY_DIR}/csound_scoparse.c)
#set(YACC_SCOOUTH ${CMAKE_CURRENT_BINARY_DIR}/csound_scoparse.h)

#set(LEX_SCOSRC ${CMAKE_CURRENT_SOURCE_DIR}/Engine/csound_sco.lex)
#set(LEX_SCOOUT ${CMAKE_CURRENT_BINARY_DIR}/csound_scolex.c)

set(PRELEX_SCOSRC ${CMAKE_CURRENT_SOURCE_DIR}/Engine/csound_prs.lex)
set(PRELEX_SCOOUT ${CMAKE_CURRENT_BINARY_DIR}/csound_prslex.c)

##message("${CMAKE_CURRENT_BINARY_DIR}/csound_prslex.c")
##message("${CMAKE_CURRENT_BINARY_DIR}/csound_scolex.c")
##message("${CMAKE_CURRENT_BINARY_DIR}/csound_scoparse.c")

#add_custom_command(
#    OUTPUT ${LEX_SCOOUT}
#    DEPENDS ${LEX_SCOSRC}
#    COMMAND ${FLEX_EXECUTABLE} ARGS -B -t ${LEX_SCOSRC} > ${LEX_SCOOUT}
#    )

add_custom_command(
    OUTPUT ${PRELEX_SCOOUT}
    DEPENDS ${PRELEX_SCOSRC}
    COMMAND ${FLEX_EXECUTABLE} ARGS -B -t -d ${PRELEX_SCOSRC} > ${PRELEX_SCOOUT}
)

#add_custom_command(
#    OUTPUT ${YACC_SCOOUT} ${YACC_SCOOUTH}
#    DEPENDS ${YACC_SCOSRC} ${LEX_SCOOUT}
#    COMMAND ${BISON_EXECUTABLE}
#    ARGS -pcsound_sco -t -d --report=itemset -o ${YACC_SCOOUT} ${YACC_SCOSRC}
#    )

list(APPEND libcsound_SRCS ${PRELEX_SCOOUT})
    #${LEX_SCOOUT} ${YACC_SCOOUT} ${PRELEX_SCOOUT})

list(APPEND libcsound_static_SRCS ${PRELEX_SCOOUT})
    #${LEX_SCOOUT} ${YACC_SCOOUT} ${PRELEX_SCOOUT})

#set_source_files_properties(${YACC_SCOOUT} GENERATED)
#set_source_files_properties(${LEX_SCOOUT} GENERATED)
set_source_files_properties(${PRELEX_SCOOUT} GENERATED)
if(MSVC)
    set_source_files_properties(${PRELEX_SCOOUT} PROPERTIES
        COMPILE_FLAGS -DYY_NO_UNISTD_H)
endif()

if(NEW_PARSER_DEBUG)
    message(STATUS "Building with new parser debugging.")
    list(APPEND libcsound_CFLAGS -DPARSER_DEBUG=1)
else()
    message(STATUS "Not building with new parser debugging.")
endif()

if(BUILD_MULTI_CORE)
    message(STATUS "Building with multicore support.")

    list(APPEND libcsound_SRCS
        Engine/cs_new_dispatch.c
        Engine/cs_par_base.c
        Engine/cs_par_orc_semantic_analysis.c)

    list(APPEND libcsound_static_SRCS
        Engine/cs_new_dispatch.c
        Engine/cs_par_base.c
        Engine/cs_par_orc_semantic_analysis.c)

    list(APPEND libcsound_CFLAGS -DPARCS)

   if(USE_LOCK_BARRIER)
     message(STATUS "Using thread-lock barrier.")
     list(APPEND libcsound_CFLAGS -DPARCS_USE_LOCK_BARRIER)
   else()
    message(STATUS "Using lock-free barrier.")
   endif()

else()
    message(STATUS "Not building with multicore support.")
endif()

set(CSOUNDLIB_STATIC "${CSOUNDLIB}-static")

# ADDING HERE TO GRAB LIST OF HEADERS IN CASE OF BUILDING OSX FRAMEWORK
add_subdirectory(include)

if(APPLE)
    string(REGEX REPLACE "([^;]+)(;|$)" "include/\\1\\2" csheaders "${csheaders}")
    file(GLOB H_headers "${CMAKE_CURRENT_SOURCE_DIR}/H/*.h")
    set(libcsound_SRCS "${libcsound_SRCS};${csheaders};${H_headers};${CMAKE_CURRENT_BINARY_DIR}/include/float-version.h;${CMAKE_CURRENT_BINARY_DIR}/include/version.h")
endif()


if(NOT IOS OR OSXCROSS_TARGET)
    set(_csound_library_type SHARED)
    if(CMAKE_SYSTEM_NAME_UPPER STREQUAL "WASI")
        set(_csound_library_type STATIC)
    endif()

    if(NOT BARE_METAL)
        add_library(${CSOUNDLIB} ${_csound_library_type} ${libcsound_SRCS})
    else()
       add_library(${CSOUNDLIB} STATIC ${libcsound_SRCS})
    endif()
    target_include_directories(${CSOUNDLIB} PRIVATE ${libcsound_private_include_dirs})
    target_include_directories(${CSOUNDLIB} PUBLIC ${libcsound_public_include_dirs})
    set_target_properties(${CSOUNDLIB} PROPERTIES SOVERSION ${APIVERSION})
endif()




if(APPLE)
    if(NOT IOS OR OSXCROSS_TARGET)
        target_link_options(${CSOUNDLIB} PRIVATE LINKER:-adhoc_codesign)
        set_target_properties(${CSOUNDLIB} PROPERTIES FRAMEWORK YES)
        set_target_properties(${CSOUNDLIB} PROPERTIES FRAMEWORK_VERSION "${APIVERSION}")
        set_target_properties(${CSOUNDLIB} PROPERTIES PUBLIC_HEADER
            "${csheaders};${CMAKE_CURRENT_BINARY_DIR}/include/float-version.h;${CMAKE_CURRENT_BINARY_DIR}/include/version.h"
        )
    endif()
endif()

if(BUILD_PLUGINS)
    message(STATUS "build external opcodes as plugins")
    target_compile_definitions(${CSOUNDLIB} PUBLIC
    "-DBUILD_PLUGINS")
endif()

# Add the install target
if(WIN32)
    install(TARGETS ${CSOUNDLIB}
        EXPORT CsoundPackageExports
        RUNTIME DESTINATION "${EXECUTABLE_INSTALL_DIR}"
        ARCHIVE DESTINATION "${LIBRARY_INSTALL_DIR}"
        FRAMEWORK DESTINATION "${CS_FRAMEWORK_DEST}")
else()
    if(NOT IOS OR OSXCROSS_TARGET)
        install(TARGETS ${CSOUNDLIB}
            EXPORT CsoundPackageExports
            LIBRARY DESTINATION "${LIBRARY_INSTALL_DIR}"
            ARCHIVE DESTINATION "${LIBRARY_INSTALL_DIR}"
            FRAMEWORK DESTINATION "${CS_FRAMEWORK_DEST}")
    endif()
endif()

if (LINUX)
    set_target_properties(${CSOUNDLIB} PROPERTIES
        INSTALL_RPATH "$ORIGIN"
    )
endif()

set(libcsound_LIBS "")

if(HAVE_WASMTIME)
    list(APPEND libcsound_LIBS Wasmtime::wasmtime)
    list(APPEND libcsound_CFLAGS -DHAVE_WASMTIME)
    set(CSOUND_PRIVATE_LIBS "${CSOUND_PRIVATE_LIBS} -lwasmtime")
endif()

if(REQUIRE_PTHREADS AND (NOT CMAKE_SYSTEM_NAME_UPPER STREQUAL "WASI" OR BUILD_WASI_REALTIME))
    list(APPEND libcsound_LIBS Threads::Threads)
endif()

if (HAVE_ATOMIC_BUILTIN)
    list(APPEND libcsound_LIBS ${ATOMIC_LIB})
    set(CSOUND_PRIVATE_LIBS "${CSOUND_PRIVATE_LIBS} ${ATOMIC_LIB}")
endif()

if(WIN32)
    list(APPEND libcsound_LIBS "${CSOUND_WINDOWS_LIBRARIES}")
endif()

if(USE_CURL AND CURL_FOUND)
  list(APPEND libcsound_LIBS ${CURL_LIBRARIES})
  set(CSOUND_PRIVATE_LIBS "${CSOUND_PRIVATE_LIBS} ${CURL_LIBRARIES}")
endif()

check_deps(USE_GETTEXT Intl_FOUND GETTEXT_FOUND)
if(USE_GETTEXT)
    message(STATUS "Using GNU gettext.")
    list(APPEND libcsound_LIBS Intl::Intl)
    list(APPEND libcsound_CFLAGS -DGNU_GETTEXT)
    set(CSOUND_PRIVATE_LIBS "${CSOUND_PRIVATE_LIBS} -lintl")
    if(NOT DEFINED CSOUND_TEXTDOMAIN AND BUILD_RELEASE AND (LINUX))
        file(TO_NATIVE_PATH "${CMAKE_INSTALL_PREFIX}/${LOCALE_INSTALL_DIR}" CSOUND_TEXTDOMAIN)
    endif()
    if(CSOUND_TEXTDOMAIN)
        message(STATUS "Setting textdomain to ${CSOUND_TEXTDOMAIN}.")
        set_source_files_properties(Top/getstring.c PROPERTIES
            COMPILE_FLAGS -DCSOUND_TEXTDOMAIN=\\\"${CSOUND_TEXTDOMAIN}\\\")
    else()
        message(STATUS "Not setting textdomain.")
    endif()
else()
    message(STATUS "Not using localization.")
endif()

if (HAVE_EXECINFO_H)
    find_library(LIBEXECINFO_LIBRARY execinfo)

    if(LIBEXECINFO_LIBRARY)
       list(APPEND libcsound_LIBS ${LIBEXECINFO_LIBRARY})
       set(CSOUND_PRIVATE_LIBS "${CSOUND_PRIVATE_LIBS} ${LIBEXECINFO_LIBRARY}")
       message(STATUS "  ADDING LIBEXECINFO LIBRARY: ${LIBEXECINFO_LIBRARY}.")
    endif()
endif()

if(LINUX)
    message(STATUS "Building on Linux.")
    add_definitions(-DLINUX -DPIPES -DNO_FLTK_THREADS -D_GNU_SOURCE
    -DHAVE_SOCKETS)
    list(APPEND libcsound_LIBS ${MATH_LIBRARY} dl)
    set(CSOUND_PRIVATE_LIBS "${CSOUND_PRIVATE_LIBS} -lm -ldl -lpthread")

    find_library(LIBRT_LIBRARY rt)

    if(LIBRT_LIBRARY)
        list(APPEND libcsound_LIBS ${LIBRT_LIBRARY})
        set(CSOUND_PRIVATE_LIBS "${CSOUND_PRIVATE_LIBS} ${LIBRT_LIBRARY}")
        message(STATUS "  ADDING LIBRT LIBRARY: ${LIBRT_LIBRARY}.")
    endif()
endif()

if(BSD)
    message(STATUS "Building on BSD.")
    add_definitions(-DBSD)
endif()

if((APPLE AND NOT IOS) OR (OSXCROSS_TARGET))
    message(STATUS "Building on OSX")
    add_definitions(-DMACOSX -DPIPES -DNO_FLTK_THREADS -DHAVE_SOCKETS)
    find_library(ACCELERATE_LIBRARY Accelerate)
    find_path(VECLIB_PATH "Accelerate/Accelerate.h")
    list(APPEND libcsound_private_include_dirs ${VECLIB_PATH})
    list(APPEND libcsound_LIBS ${MATH_LIBRARY} dl ${ACCELERATE_LIBRARY})
    if(APPLE AND NOT IOS)
        set(CSOUND_PRIVATE_LIBS "${CSOUND_PRIVATE_LIBS} -lm -ldl ${ACCELERATE_LIBRARY}")
    endif()
endif()

if(WIN32)
    add_definitions(-DWIN32)
endif()

check_function_exists(strlcat HAVE_STRLCAT)
if(HAVE_STRLCAT)
    add_definitions(-DHAVE_STRLCAT)
endif()

# Locale-aware reading and printing
check_function_exists(strtok_r HAVE_STRTOK_R)
check_function_exists(strtod_l HAVE_STRTOD_L)
check_function_exists(sprintf_l HAVE_SPRINTF_L)

if(HAVE_STRTOK_R)
    add_definitions(-DHAVE_STRTOK_R)
endif()
if(HAVE_STRTOD_L AND NOT EMSCRIPTEN)
    add_definitions(-DHAVE_STRTOD_L)
endif()
if(HAVE_SPRINTF_L)
    add_definitions(-DHAVE_SPRINTF_L)
endif()

# Same for Windows
check_function_exists(_strtok_r HAVE__STRTOK_R)
check_function_exists(_strtod_l HAVE__STRTOD_L)
check_function_exists(_sprintf_l HAVE__SPRINTF_L)

if(HAVE__STRTOK_R)
    add_definitions(-DHAVE__STRTOK_R)
endif()
if(HAVE__STRTOD_L)
    add_definitions(-DHAVE__STRTOD_L)
endif()
if(HAVE__SPRINTF_L)
    add_definitions(-DHAVE__SPRINTF_L)
endif()

# Pass flags according to system capabilities

if((HAVE_WINSOCK_H OR HAVE_SYS_SOCKET_H) AND
   NOT EMSCRIPTEN AND
   NOT CMAKE_SYSTEM_NAME_UPPER STREQUAL "WASI")
    add_definitions(-DHAVE_SOCKETS)
endif()

#if(CMAKE_C_COMPILER MATCHES "gcc")
#    list(APPEND libcsound_CFLAGS -fno-strict-aliasing)
#endif()

if(BIG_ENDIAN)
    list(APPEND libcsound_CFLAGS -DWORDS_BIGENDIAN)
endif()

if(NOT IOS OR OSXCROSS_TARGET)
    target_compile_options(${CSOUNDLIB} PRIVATE ${libcsound_CFLAGS})
endif()

if(MSVC)
    target_link_libraries(${CSOUNDLIB} PRIVATE ws2_32)
    if(HAVE_WASMTIME)
        target_link_libraries(${CSOUNDLIB} PRIVATE Wasmtime::wasmtime)
    endif()
else()
    if(NOT IOS OR OSXCROSS_TARGET)
        target_link_libraries(${CSOUNDLIB} PRIVATE ${libcsound_LIBS})
    endif()
endif()

if(USE_LIBSNDFILE)
    if(NOT IOS OR OSXCROSS_TARGET)
        target_link_libraries(${CSOUNDLIB} PRIVATE SndFile::sndfile)
       if(USE_STATIC_DEPS)
          message(STATUS "Adding static link libraries for libsndfile:")
          Find_Package(Oggs)
          if(Oggs_FOUND)
           message(STATUS "Found ogg:${Oggs_LIBRARY}")
          else()
           message(STATUS "Ogg not found")
          endif()
          Find_Package(Vorbiss)
          if(Vorbiss_FOUND)
           message(STATUS "Found vorbis:${Vorbiss_LIBRARY}")
          else()
           message(STATUS "Vorbis not found")
          endif()
          Find_Package(Vorbisencs)
          if(Vorbisencs_FOUND)
           message(STATUS "Found vorbisenc:${Vorbisencs_LIBRARY}")
          else()
           message(STATUS "Vorbisenc not found")
          endif()
          Find_Package(Opuss)
          if(Opuss_FOUND)
           message(STATUS "Found opus:${Opuss_LIBRARY}")
          else()
           message(STATUS "Opus not found")
          endif()
          Find_Package(FLACS)
          if(FLACS_FOUND)
           message(STATUS "Found flac:${FLACS_LIBRARY}")
          else()
           message(STATUS "FLAC not found")
          endif()
          Find_Package(LAME)
           if(LAME_FOUND)
           message(STATUS "Found lame:${LAME_LIBRARY}")
          else()
           message(STATUS "LAME not found")
          endif()
          Find_Package(Mpg123s)
          if(Mpg123s_FOUND)
           message(STATUS "Found mpg123:${Mpg123s_LIBRARY}")
          else()
           message(STATUS "Mpg123 not found")
          endif()
          target_link_libraries(${CSOUNDLIB} PRIVATE Oggs::oggs)
          target_link_libraries(${CSOUNDLIB} PRIVATE Vorbiss::vorbiss)
          target_link_libraries(${CSOUNDLIB} PRIVATE Vorbisencs::vorbisencs)
          target_link_libraries(${CSOUNDLIB} PRIVATE FLACS::flacs)
          target_link_libraries(${CSOUNDLIB} PRIVATE Opuss::opuss)
          target_link_libraries(${CSOUNDLIB} PRIVATE "${LAME_LIBRARY}")
          target_link_libraries(${CSOUNDLIB} PRIVATE Mpg123s::mpg123s)
       endif()
    endif()
endif()

if(SampleRate_FOUND)
    if(NOT IOS OR OSXCROSS_TARGET)
        target_link_libraries(${CSOUNDLIB} PRIVATE SampleRate::samplerate)
    endif()
    set(CSOUND_PRIVATE_LIBS "${CSOUND_PRIVATE_LIBS} -lsamplerate")
endif()

if(NOT IOS OR OSXCROSS_TARGET)
    set_target_properties(${CSOUNDLIB} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR})
endif()



if(BUILD_STATIC_LIBRARY AND NOT BARE_METAL)
    add_library(${CSOUNDLIB_STATIC} STATIC ${libcsound_static_SRCS})
    target_include_directories(${CSOUNDLIB_STATIC} PRIVATE ${libcsound_private_include_dirs})
    target_include_directories(${CSOUNDLIB_STATIC} PUBLIC ${libcsound_public_include_dirs})
    SET_TARGET_PROPERTIES(${CSOUNDLIB_STATIC} PROPERTIES OUTPUT_NAME ${CSOUNDLIB})
    SET_TARGET_PROPERTIES(${CSOUNDLIB_STATIC} PROPERTIES PREFIX "lib")
    if(BUILD_PLUGINS)
     list(APPEND libcsound_CFLAGS -DCS_INTERNAL)
     endif()
    target_compile_options(${CSOUNDLIB_STATIC} PRIVATE
    ${libcsound_CFLAGS})

    if (USE_LIBSNDFILE)
        target_link_libraries(${CSOUNDLIB_STATIC} PUBLIC SndFile::sndfile)
    endif()

    if(SampleRate_FOUND)
        target_link_libraries(${CSOUNDLIB_STATIC} PUBLIC SampleRate::samplerate)
    endif()

    target_link_libraries(${CSOUNDLIB_STATIC} PUBLIC ${libcsound_LIBS})

    set_target_properties(${CSOUNDLIB_STATIC} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR})

    if (LINUX)
        set_target_properties(${CSOUNDLIB_STATIC} PROPERTIES
            INSTALL_RPATH "$ORIGIN"
        )
    endif()

    # Add the install target
    install(TARGETS ${CSOUNDLIB_STATIC}
        EXPORT CsoundStaticExports
        LIBRARY DESTINATION "${LIBRARY_INSTALL_DIR}"
        ARCHIVE DESTINATION "${LIBRARY_INSTALL_DIR}")
endif()

csound_suppress_opcode_initializer_warnings(${libcsound_SRCS} ${libcsound_static_SRCS})

add_subdirectory(Opcodes)
add_subdirectory(InOut)
add_subdirectory(Frontends)
add_subdirectory(languages/python)
add_subdirectory(platform/bela)
add_subdirectory(util)
add_subdirectory(util1)
add_subdirectory(po)
add_subdirectory(tests/c)
add_subdirectory(tests/benchmarks)
add_subdirectory(tests/commandline)
add_subdirectory(tests/regression)
add_subdirectory(tests/soak)
add_subdirectory(tests/python)

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

# target etags/tags
if(UNIX)
    add_custom_target(tags COMMAND etags `find ${CMAKE_CURRENT_SOURCE_DIR} -name \\*.cc -or -name \\*.hh -or -name \\*.cpp -or -name \\*.h -or -name \\*.c | grep -v " " `)
    add_custom_target(etags DEPENDS tags)
endif()

# build a CPack driven installer package
#include (InstallRequiredSystemLibraries)

set (CPACK_PACKAGE_NAME "csound")
set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set (CPACK_PACKAGE_VERSION "7.0.0")
set (CPACK_PACKAGE_VERSION_MAJOR "7")
set (CPACK_PACKAGE_VERSION_MINOR "0")
set (CPACK_PACKAGE_VERSION_PATCH "0")
include (CPack)

# Documentation
if(BUILD_DOCS)
    find_package(Doxygen)

    if(DOXYGEN_FOUND)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/config.doxygen
            ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY
        )

        add_custom_target(doc
            COMMAND ${CMAKE_COMMAND} -E make_directory
                ${CMAKE_CURRENT_BINARY_DIR}/docs/doxygen
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    else()
        message(STATUS "Not building documentation")
    endif()
else()
    message(STATUS "Not building documentation")
endif()

# install CMake package configuration files
include(CMakePackageConfigHelpers)

# Match the Windows installer layout so exported paths remain relative.
if(WIN32)
    set(CSOUND_CMAKE_INSTALL_DIR "cmake")
else()
    set(CSOUND_CMAKE_INSTALL_DIR "share/csound")
endif()

# export targets to CsoundTargets.cmake (separate from config wrapper)
if(WIN32 OR NOT IOS OR OSXCROSS_TARGET)
    install(EXPORT CsoundPackageExports
        FILE "CsoundTargets.cmake"
        NAMESPACE "Csound::"
        DESTINATION "${CSOUND_CMAKE_INSTALL_DIR}"
    )
endif()

# export static library targets (only when BUILD_STATIC_LIBRARY is enabled)
if(BUILD_STATIC_LIBRARY AND NOT BARE_METAL)
    install(EXPORT CsoundStaticExports
        FILE "CsoundStaticTargets.cmake"
        NAMESPACE "Csound::"
        DESTINATION "${CSOUND_CMAKE_INSTALL_DIR}"
    )
endif()

# generate relocatable CsoundConfig.cmake from template
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CsoundConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CsoundConfig.cmake"
    INSTALL_DESTINATION "${CSOUND_CMAKE_INSTALL_DIR}"
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/CsoundConfig.cmake"
    DESTINATION "${CSOUND_CMAKE_INSTALL_DIR}"
)

# generate and install CsoundConfigVersion.cmake
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/CsoundConfigVersion.cmake"
    VERSION "${CS_VERSION}.${CS_SUBVER}.${CS_PATCHLEVEL}"
    COMPATIBILITY AnyNewerVersion
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/CsoundConfigVersion.cmake"
    DESTINATION "${CSOUND_CMAKE_INSTALL_DIR}"
)

# install FindCsound.cmake for projects using Module mode
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindCsound.cmake"
    DESTINATION "${CSOUND_CMAKE_INSTALL_DIR}"
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/FindWasmtime.cmake"
    DESTINATION "${CSOUND_CMAKE_INSTALL_DIR}"
)

# configure and install pkg-config file
if(IS_ABSOLUTE "${LIBRARY_INSTALL_DIR}")
    set(CSOUND_PKGCONFIG_LIBDIR "${LIBRARY_INSTALL_DIR}")
else()
    set(CSOUND_PKGCONFIG_LIBDIR "\${prefix}/${LIBRARY_INSTALL_DIR}")
endif()

# Set appropriate link flags for different platforms
if(APPLE AND NOT IOS)
    # On macOS, use framework flags when building as framework
    set(CSOUND_LINK_FLAGS "-F${CS_FRAMEWORK_DEST} -framework ${CSOUNDLIB}")
else()
    # Standard library flags for other platforms
    set(CSOUND_LINK_FLAGS "-L\${libdir} -l${CSOUNDLIB}")
endif()


configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/csound.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/csound.pc"
    @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/csound.pc"
    DESTINATION "${LIBRARY_INSTALL_DIR}/pkgconfig"
)

# install samples
if(APPLE)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/samples
            DESTINATION  ${CS_FRAMEWORK_DEST}/${CSOUNDLIB}.framework/Versions/${APIVERSION})
else()
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/samples
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share)
endif()

# install mingw libraries
if(MINGW)
    # Detect 32-bit vs 64-bit MinGW for runtime DLL paths
    if (CMAKE_C_COMPILER MATCHES "i686-w64-mingw32")
        # 32-bit MinGW
        file(GLOB LIBSTDC_DLL "/usr/lib/gcc/i686-w64-mingw32/*-posix/libstdc++-6.dll ")
        file(COPY ${LIBSTDC_DLL} DESTINATION "${BUILD_DIR}")
        file(GLOB LIBGCC_DLL "/usr/lib/gcc/i686-w64-mingw32/*-posix/libgcc_s_dw2-1.dll ")
        file(COPY ${LIBGCC_DLL} DESTINATION "${BUILD_DIR}")
        file(COPY /usr/i686-w64-mingw32/lib/libwinpthread-1.dll DESTINATION "${BUILD_DIR}")
    else()
        # 64-bit MinGW
        file(GLOB LIBSTDC_DLL "/usr/lib/gcc/x86_64-w64-mingw32/*-posix/libstdc++-6.dll ")
        file(COPY ${LIBSTDC_DLL} DESTINATION "${BUILD_DIR}")
        file(GLOB LIBGCC_DLL "/usr/lib/gcc/x86_64-w64-mingw32/*-posix/libgcc_s_seh-1.dll ")
        file(COPY ${LIBGCC_DLL} DESTINATION "${BUILD_DIR}")
        file(COPY /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll DESTINATION "${BUILD_DIR}")
    endif()
endif()

add_custom_target(format sh scripts/format.sh  Top/*.c  Top/*.cpp  Engine/*.c
    OOps/*.c Opcodes/*.c  Opcodes/*.cpp Opcodes/*/*.c  Opcodes/*/*.cpp
    util/*.c WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

if(BUILD_WASI_REALTIME AND NOT BUILD_WASI_BROWSER)
    add_subdirectory(platform/wasm-wasi/tests/realtime)
endif()
