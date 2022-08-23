cmake_minimum_required(VERSION 3.5)

# silence RPATH cmake warning
set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

option(USE_VCPKG "Use VCPKG to download and manage dependencies" OFF)

if (USE_VCPKG)
    # Detect correct triplet based on CMake env
    if (WIN32)
        set(VCPKG_TARGET_TRIPLET "x64-windows-csound")
    elseif(APPLE)
        set(VCPKG_TARGET_TRIPLET "x64-osx")
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

ENABLE_TESTING()

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set(CMAKE_COMPILER_IS_CLANG 1)
endif()

message(STATUS "Csound directory: ${CMAKE_HOME_DIRECTORY}")

# C++11 needed
if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# Project definitions
set(APIVERSION "7.0")
set(CS_VERSION "7")
set(CS_SUBVER "0")
set(CS_PATCHLEVEL "0")

message(STATUS "Csound version:
${CS_VERSION}.${CS_SUBVER}.${CS_PATCHLEVEL}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/README.md.in ${CMAKE_CURRENT_SOURCE_DIR}/README.md)

set(MACOSX_FRAMEWORK_BUNDLE_VERSION
"${CS_VERSION}.${CS_SUBVER}.${CS_PATCHLEVEL}")

set(MACOSX_FRAMEWORK_SHORT_VERSION_STRING
"${CS_VERSION}.${CS_SUBVER}.${CS_PATCHLEVEL}")

set(MACOSX_FRAMEWORK_IDENTIFIER "CSND")

# Relative install paths
set(EXECUTABLE_INSTALL_DIR "bin")
set(LOCALE_INSTALL_DIR "share/locale")
set(HEADER_INSTALL_DIR "include/csound")
set(SAMPLES_INSTALL_DIR "/samples")
set(CS_FRAMEWORK_DEST "$ENV{HOME}/Library/Frameworks" CACHE PATH
"Csound framework path")

include(TestBigEndian)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckLibraryExists)
include(CMakeParseArguments)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CMakePushCheckState)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules/)
message(STATUS "CMake module path: ${CMAKE_MODULE_PATH}")

# Utility to munge with items in a list.
#
macro(LIST_REPLACE LIST INDEX NEWVALUE)
    list(INSERT ${LIST} ${INDEX} ${NEWVALUE})
    MATH(EXPR __INDEX "${INDEX} + 1")
    list (REMOVE_AT ${LIST} ${__INDEX})
endmacro(LIST_REPLACE)

# Utility function to make executables. All plugin targets should use this as it
# sets up output directory set in top-level CmakeLists.txt
# and adds an appropriate install target
#
# name - name of executable to produce
# srcs - list of src files
# libs - list of library files to link to
# output_name (OPTIONAL) - overide the name of the generated executable
#
function(make_executable name srcs libs)
    add_executable(${name} ${srcs})
    target_link_libraries (${name} ${libs})
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR})

    if(${ARGC} EQUAL 4)
        set_target_properties(${name} PROPERTIES
            OUTPUT_NAME ${ARGV3})
    endif()
    install(TARGETS ${name}
	RUNTIME DESTINATION "${EXECUTABLE_INSTALL_DIR}" )
endfunction(make_executable)


# Utility function to make a utility executable
#
# name - name of executable to produce
# srcs - list of src files
# libraries - OPTIONAL extra libraries to link
#
function(make_utility name srcs)
    make_executable(${name} "${srcs}" "${CSOUNDLIB}")
    add_dependencies(${name} ${CSOUNDLIB})
    set(i 2)
    while( ${i} LESS ${ARGC} )
        target_link_libraries(${name} ${ARGV${i}})
        math(EXPR i "${i}+1")
    endwhile()
endfunction()

# Checks if dependencies for an enabled target are fulfilled.
# If FAIL_MISSING is true and the dependencies are not fulfilled,
# it will abort the cmake run.
# If FAIL_MISSING is false, it will set the option to OFF.
# If the target is not enabled, it will do nothing.
# example: check_deps(BUILD_NEW_PARSER FLEX_EXECUTABLE
# BISON_EXECUTABLE)
#
function(check_deps option)
    if(${option})
        set(i 1)
        while( ${i} LESS ${ARGC} )
            set(dep ${ARGV${i}})
            if(NOT ${dep})
                if(FAIL_MISSING)
                    message(FATAL_ERROR
                        "${option} is enabled, but ${dep}=\"${${dep}}\"")
                else()
                    message(STATUS "${dep}=\"${${dep}}\", so disabling ${option}")
                    set(${option} OFF PARENT_SCOPE)
                    # Set it in the local scope too
                    set(${option} OFF)
                endif()
            endif()
            math(EXPR i "${i}+1")
        endwhile()
    endif()
    if(${option})
        message(STATUS "${option} is enabled.")
    else()
        message(STATUS "${option} is disabled.")
    endif()
endfunction(check_deps)

### COMPILER OPTIMIZATION FLAGS
option(USE_COMPILER_OPTIMIZATIONS "Use the default Csound compiler optimization flags" ON)
if(USE_COMPILER_OPTIMIZATIONS)
include(cmake/CompilerOptimizations.cmake)
endif()

## USER OPTIONS ##
# Optional targets, they should all default to ON (check_deps will disable them if not possible to build)
option(USE_DOUBLE "Set to use double-precision floating point for audio samples." ON)
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
option(BUILD_RELEASE "Build for release" ON)
option(BUILD_INSTALLER "Build installer" OFF)
option(BUILD_TESTS "Build tests" ON)
option(LEXER_DEBUG "Build lexer with debug option" OFF)
option(USE_GIT_COMMIT "Show the git commit in version information" ON)
option(REQUIRE_PTHREADS "For non-Windows systems, set whether Csound will use threads or not" ON)

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

# in Release configuration, set NDEBUG
if(${CMAKE_BUILD_TYPE} MATCHES "Release")
    message(STATUS "Building for release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNDEBUG")
elseif(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    message(STATUS "Building for debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBETA")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DBETA")
endif()

# set -Werror if in Debug configuration
if(NOT MSVC AND NOT WASM)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 ")
    set(CMAKE_C_FLAGS_RELEASE "-O3 ")
    if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wall -Werror -Wno-missing-field-initializers")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wno-missing-field-initializers")
    endif()
endif()

if(APPLE)
    list(APPEND CMAKE_REQUIRED_INCLUDES "time.h")
    CHECK_FUNCTION_EXISTS(clock_gettime CLOCK_GETTIME)
    if(${CLOCK_GETTIME})
        message(STATUS  "clock_gettime() found")
        set(CMAKE_C_FLAGS "-DHAVE_CLOCK_GETTIME ${CMAKE_C_FLAGS}")
    else()
        message(STATUS  "clock_gettime() not found")
    endif()
endif()

# library installation directories
option(USE_LIB64 "Set to on to set installation directory for libraries to lib64" OFF)
if(USE_LIB64)
	set(LIBRARY_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib64")
    add_definitions("-DLIB64")
elseif(NOT DEFINED LIBRARY_INSTALL_DIR)
    set(LIBRARY_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib")
endif()
message(STATUS "LIBRARY INSTALL DIR: ${LIBRARY_INSTALL_DIR}")

if(USE_DOUBLE)
    message(STATUS "Building with 64-bit floats")
    set(PLUGIN_INSTALL_DIR "${LIBRARY_INSTALL_DIR}/csound/plugins64-${APIVERSION}")
else()
    message(STATUS "Building with 32-bit floats")
    set(PLUGIN_INSTALL_DIR "${LIBRARY_INSTALL_DIR}/csound/plugins-${APIVERSION}")
endif()

if(WIN32 AND NOT MSVC)
    if(EXISTS "C:/MinGW/include")
        include_directories(C:/MinGW/include)
    else()
        MESSAGE(STATUS "MinGW include dir not found.")
    endif()
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--add-stdcall-alias")
endif()

if(WIN32)
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
    set(CMAKE_SHARED_MODULE_PREFIX "")

    set(CSOUND_WINDOWS_LIBRARIES
        advapi32
        comctl32
        comdlg32
        glu32
        kernel32
        odbc32
        odbccp32
        ole32
        oleaut32
        shell32
        user32
        uuid
        winmm
        winspool
        ws2_32
        wsock32)

    if(MSVC)
        # Experimented with flags but did not make change performance results
        #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Oi /fp:fast /arch:AVX2")
        #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Oi /fp:fast /arch:AVX2")

        # Replace the default CRT linkage from dynamic (MD) to static (MT)
        if(STATIC_CRT STREQUAL "ON")
            foreach(flag_var
                CMAKE_C_FLAGS CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_DEBUG
                CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
                string(REPLACE "/MD" "-MT" ${flag_var} "${${flag_var}}")
            endforeach()
            message(STATUS "Using static CRT linkage /MT")
        endif()
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mstackrealign -static-libgcc -static")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mstackrealign -static-libstdc++ -static-libgcc -static")
        set_target_properties(${CSOUNDLIB} PROPERTIES LINK_FLAGS "-static-libstdc++ -static-libgcc -static")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libstdc++ -static-libgcc -static")
    endif()
endif()

# MacOS/IOS c++11 flags
if(APPLE)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set_target_properties(${CSOUNDLIB} PROPERTIES CXX_COMPILE_FLAGS  "-std=c++11")
endif()

set(BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

check_c_compiler_flag(-fvisibility=hidden HAS_VISIBILITY_HIDDEN)
check_cxx_compiler_flag(-fvisibility=hidden HAS_CXX_VISIBILITY_HIDDEN)
if (HAS_VISIBILITY_HIDDEN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
endif()
if (HAS_CXX_VISIBILITY_HIDDEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
endif()

check_c_compiler_flag(-std=gnu99 HAS_GNU99)
if (HAS_GNU99)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
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

  if(CMAKE_OSX_SYSROOT AND NOT CMAKE_OSX_SYSROOT MATCHES ".*OSX10.6.*"
     AND (NOT  CMAKE_OSX_DEPLOYMENT_TARGET))
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.7)
    message(STATUS "OSX: Setting Deployment Target to 10.7")
  else()
    message(STATUS "OSX: Setting Deployment Target to ${CMAKE_OSX_DEPLOYMENT_TARGET}")
  endif()

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
    else()
        set(CSOUNDLIB "csound64")
    endif()
else()
    if(APPLE)
        set(CSOUNDLIB "CsoundLib")
        set(PLUGIN_INSTALL_DIR "${CS_FRAMEWORK_DEST}/${CSOUNDLIB}.framework/Versions/${APIVERSION}/Resources/Opcodes")
    else()
        set(CSOUNDLIB "csound")
    endif()
endif()

# set default user plugin directory
if(APPLE)
    if(BUILD_INSTALLER)
       	set(CS_FRAMEWORK_FULL_PATH "/Library/Frameworks/${CSOUNDLIB}.framework/Versions/${APIVERSION}/Resources/Opcodes64")
    else()
        get_filename_component(CS_FRAMEWORK_FULL_PATH ${PLUGIN_INSTALL_DIR} ABSOLUTE)
    endif()

    add_definitions("-DCS_DEFAULT_PLUGINDIR=\"${CS_FRAMEWORK_FULL_PATH}\"")
    set(DEFAULT_OPCODEDIR ${CS_FRAMEWORK_FULL_PATH})

    # dir relative to $HOME
    if(USE_DOUBLE)
      set(DEFAULT_USER_PLUGINDIR
    "Library/csound/${APIVERSION}/plugins64")
   else()
      set(DEFAULT_USER_PLUGINDIR
    "Library/csound/${APIVERSION}/plugins")
    endif()

    add_definitions("-DCS_DEFAULT_USER_PLUGINDIR=\"${DEFAULT_USER_PLUGINDIR}\"")
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
    set(DEFAULT_USER_PLUGINDIR
    ".local/lib/csound/${APIVERSION}/plugins64")
    else()
    set(DEFAULT_USER_PLUGINDIR
    ".local/lib/csound/${APIVERSION}/plugins")
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

if(BUILD_RELEASE)
    add_definitions("-D_CSOUND_RELEASE_")
else()
    add_definitions("-DBETA")
endif()

if(APPLE OR LINUX)
message(STATUS "default plugin dir set to ${DEFAULT_OPCODEDIR}")
endif()

set(BUILD_PLUGINS_DIR ${BUILD_DIR})
set(BUILD_BIN_DIR ${BUILD_DIR})
set(BUILD_LIB_DIR ${BUILD_DIR})

message(STATUS "BUILD_BIN_DIR set to ${BUILD_BIN_DIR}.")
message(STATUS "BUILD_LIB_DIR set to ${BUILD_LIB_DIR}.")
message(STATUS "BUILD_PLUGINS_DIR set to ${BUILD_PLUGINS_DIR}.")

# OS specific checks
TEST_BIG_ENDIAN(BIG_ENDIAN)

## CONFIGURATION ##
SET(BUILD_SHARED_LIBS ON)

## HEADER/LIBRARY/OTHER CHECKS ##
# First, required stuff
if(USE_LIBSNDFILE)
 set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_LIBSNDFILE")
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_LIBSNDFILE")
 if(MSVC)
    find_package(SndFile CONFIG REQUIRED)
 else()
    find_library(LIBSNDFILE_LIBRARY NAMES sndfile libsndfile-1 libsndfile)

  if(NOT LIBSNDFILE_LIBRARY)
        message(FATAL_ERROR "Csound requires the sndfile library")
   endif()
 endif()
endif()

find_package(CURL)
#set(FLTK_SKIP_OPENGL true)
#find_package(FLTK)
find_package(Java)
find_package(JNI)
#find_package(MUSICXML)
#find_package(Python)
#find_package(VSTSDK2X)

# TODO should this work for MSVC also?
if(WIN32)
  find_library(FLAC_LIB NAMES FLAC flac)
  find_library(OGG_LIB ogg)
  find_library(SPEEX_LIB speex)
  find_library(VORBIS_LIB vorbis)
  find_library(VORBISENC_LIB vorbisenc)
  set(LIBSNDFILE_SUPPORT_LIBS ${VORBIS_LIB} ${VORBISENC_LIB} ${FLAC_LIB} ${OGG_LIB})
  if(SPEEX_LIB)
    list(APPEND LIBSNDFILE_SUPPORT_LIBS ${SPEEX_LIB} )
  endif()
  #message(STATUS "${LIBSNDFILE_SUPPORT_LIBS}")
endif()

find_path(SNDFILE_H_PATH sndfile.h)
if(SNDFILE_H_PATH)
    include_directories(${SNDFILE_H_PATH})
else()
    message(FATAL_ERROR "Could not find sndfile.h")
endif()

if(NOT MSVC AND REQUIRE_PTHREADS)
  # Use ladder of if's to support older CMake versions (i.e., travis)
  find_library(PTHREAD_LIBRARY winpthread-1)
  if(NOT PTHREAD_LIBRARY)
    find_library(PTHREAD_LIBRARY pthread)
  endif()
  if(NOT PTHREAD_LIBRARY)
    find_library(PTHREAD_LIBRARY pthreadGC2)
  endif()

  if(NOT (PTHREAD_LIBRARY OR HAIKU))
      # pthreads are built in to Haiku
      message(STATUS "Csound requires the pthread library")
  endif()

  #FIXME this variable is for required include directories, possible bug
  list(APPEND CMAKE_REQUIRED_INCLUDES pthread.h)
  if (NOT HAIKU)
    set(CMAKE_REQUIRED_LIBRARIES pthread)
  endif()
endif()


# Now, non required library searches
option(USE_ATOMIC_BUILTIN "Use Atomic Builtins if supported" ON)
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

find_library(VORBISFILE_LIBRARY vorbisfile)
#check_include_file(libintl.h LIBINTL_HEADER)
find_library(LIBINTL_LIBRARY intl HINTS /usr/local/lib)
find_package(Gettext)
find_library(MATH_LIBRARY m)
if (NOT MATH_LIBRARY)
    set(MATH_LIBRARY "")
endif()

cmake_push_check_state()
list(APPEND CMAKE_REQUIRED_LIBRARIES ${MATH_LIBRARY})
check_function_exists(lrint HAVE_LRINT)
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
        message(WARNING "Dirent.h not found, Csound will not be able to load dynamic plugins such as real-time audio")
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
endforeach()

if (NOT HAVE_INTTYPES_H)
    message(FATAL_ERROR "Csound requires inttypes.h")
endif()

if (HAVE_EXECINFO_H)
    add_definitions("-DHAVE_EXECINFO")
endif()

check_deps(USE_LRINT HAVE_LRINT)
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

include_directories(./H)
include_directories(./include)
include_directories(./Engine)

#adding this for files that #include SDIF/sdif*
include_directories(./util)
#adding this for files that #include files in local dirs
include_directories(./)

#checking pthread functions
if(REQUIRE_PTHREADS AND (PTHREAD_LIBRARY OR HAIKU))

  #list(APPEND libcsound_CFLAGS -DHAVE_PTHREAD)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_PTHREAD")

  check_function_exists(pthread_spin_lock PTHREAD_SPIN_LOCK_EXISTS)
  check_function_exists(pthread_barrier_init PTHREAD_BARRIER_INIT_EXISTS)

  if(PTHREAD_SPIN_LOCK_EXISTS)
     list(APPEND libcsound_CFLAGS -DHAVE_PTHREAD_SPIN_LOCK)
  endif()

  if(PTHREAD_BARRIER_INIT_EXISTS)
     list(APPEND libcsound_CFLAGS -DHAVE_PTHREAD_BARRIER_INIT)
  endif()
endif()

include_directories(${LIBSNDFILE_INCLUDE_DIRECTORY})
# get the git hash and pass it to csound
SET(git_hash_values "none")
if(USE_GIT_COMMIT)
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
set_source_files_properties(Top/csound.c PROPERTIES
  COMPILE_FLAGS  -DGIT_HASH_VALUE=${git_hash_values})


# The csound library
set(libcsound_SRCS
   Top/csound.c
    Engine/auxfd.c
    Engine/cfgvar.c
    Engine/corfiles.c
    Engine/entry1.c
    Engine/envvar.c
    Engine/extract.c
    Engine/fgens.c
    Engine/insert.c
    Engine/linevent.c
    Engine/memalloc.c
    Engine/memfiles.c
    Engine/musmon.c
    Engine/namedins.c
    Engine/rdscor.c
    Engine/scsort.c
    Engine/scxtract.c
    Engine/sort.c
    Engine/sread.c
    Engine/swritestr.c
    Engine/twarp.c
    Engine/csound_type_system.c
    Engine/csound_standard_types.c
    Engine/csound_data_structures.c
    Engine/pools.c
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
    OOps/aops.c
    OOps/bus.c
    OOps/cmath.c
    OOps/diskin2.c
    OOps/disprep.c
    OOps/dumpf.c
    OOps/fftlib.c
    OOps/lpred.c
    OOps/pffft.c
    OOps/goto_ops.c
    OOps/midiinterop.c
    OOps/midiops.c
    OOps/midiout.c
    OOps/mxfft.c
    OOps/oscils.c
    OOps/pstream.c
    OOps/pvfileio.c
    OOps/pvsanal.c
    OOps/random.c
    OOps/remote.c
    OOps/schedule.c
    OOps/sndinfUG.c
    OOps/str_ops.c
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
    Opcodes/babo.c
    Opcodes/bilbar.c
    Opcodes/compress.c
    Opcodes/eqfil.c
    Opcodes/Vosim.c
    Opcodes/squinewave.c
    Opcodes/pinker.c
    Opcodes/pitch.c
    Opcodes/pitch0.c
    Opcodes/spectra.c
    Opcodes/ambicode1.c
    Opcodes/sfont.c
    Opcodes/grain4.c
    Opcodes/hrtferX.c
    Opcodes/loscilx.c
    Opcodes/minmax.c
    Opcodes/pan2.c
    Opcodes/arrays.c
    Opcodes/phisem.c
    Opcodes/hrtfopcodes.c
    Opcodes/vbap.c
    Opcodes/vbap1.c
    Opcodes/vbap_n.c
    Opcodes/vbap_zak.c
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
    Opcodes/modal4.c
    Opcodes/physutil.c
    Opcodes/physmod.c
    Opcodes/mandolin.c
    Opcodes/singwave.c
    Opcodes/fm4op.c
    Opcodes/moog1.c
    Opcodes/shaker.c
    Opcodes/bowedbar.c
    Opcodes/gab/tabmorph.c
    Opcodes/gab/hvs.c
    Opcodes/gab/sliderTable.c
    Opcodes/gab/newgabopc.c
    Opcodes/ftest.c
    Opcodes/hrtfearly.c
    Opcodes/hrtfreverb.c
    Opcodes/cpumeter.c
    Opcodes/gendy.c
    Opcodes/tl/sc_noise.c
    Opcodes/afilters.c
    Opcodes/wpfilters.c
    Opcodes/zak.c
    Opcodes/lufs.c
    Opcodes/date.c
    Opcodes/system_call.c
    Opcodes/liveconv.c
    Opcodes/sterrain.c
    Opcodes/gammatone.c
    Opcodes/fareygen.c
    Opcodes/quadbezier.c
    Opcodes/framebuffer.c
    Opcodes/cellular.c
    Opcodes/exciter.c
    Opcodes/buchla.c
    Opcodes/select.c
    Opcodes/serial.c
    Opcodes/counter.c
    Opcodes/platerev.c
    Opcodes/sequencer.c
    Opcodes/pvsgendy.c
    Opcodes/emugens/emugens.c
    Opcodes/emugens/scugens.c
    Top/argdecode.c
    Top/csdebug.c
    Top/cscore_internal.c
    Top/cscorfns.c
    Top/csmodule.c
    Top/getstring.c
    Top/main.c
    Top/new_opts.c
    Top/one_file.c
    Top/opcode.c
    Top/threads.c
    Top/utility.c
    Top/threadsafe.c
    Top/server.c)

if(WIN32 AND NOT MSVC)
set_source_files_properties(Opcodes/sfont.c PROPERTIES
    COMPILE_FLAGS -mno-ms-bitfields)
endif()

if(NOT MSVC)
set_source_files_properties(Opcodes/sfont.c PROPERTIES
  COMPILE_FLAGS -Wno-address-of-packed-member)
endif()



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
    Opcodes/midiops2.c
    Opcodes/midiops3.c
    Opcodes/newfils.c
    Opcodes/nlfilt.c
    Opcodes/oscbnk.c
    Opcodes/pluck.c
    Opcodes/paulstretch.c
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
    Opcodes/socksend.c
    Opcodes/sockrecv.c)

set(cs_pvs_ops_SRCS
    Opcodes/ifd.c
    Opcodes/partials.c
    Opcodes/psynth.c
    Opcodes/pvsbasic.c
    Opcodes/pvscent.c
    Opcodes/pvsdemix.c
    Opcodes/pvs_ops.c
    Opcodes/pvsband.c
    Opcodes/pvsbuffer.c)

set(oldpvoc_SRCS
    Opcodes/dsputil.c
    Opcodes/pvadd.c
    Opcodes/pvinterp.c
    Opcodes/pvocext.c
    Opcodes/pvread.c
    Opcodes/ugens8.c
    Opcodes/vpvoc.c
    Opcodes/pvoc.c)

set(mp3in_SRCS
    Opcodes/mp3in.c
    InOut/libmpadec/layer1.c
    InOut/libmpadec/layer2.c
    InOut/libmpadec/layer3.c
    InOut/libmpadec/synth.c
    InOut/libmpadec/tables.c
    InOut/libmpadec/mpadec.c
    InOut/libmpadec/mp3dec.c)

list(APPEND libcsound_SRCS ${stdopcod_SRCS} ${cs_pvs_ops_SRCS}
${oldpvoc_SRCS} ${mp3in_SRCS})

if(BUILD_PERFTHREAD_CLASS)
 list(APPEND libcsound_SRCS "Top/csPerfThread.cpp")
endif()

if(INIT_STATIC_MODULES)
    set(static_modules_SRCS
        Top/init_static_modules.c
        Opcodes/ampmidid.cpp
        Opcodes/doppler.cpp
        Opcodes/tl/fractalnoise.cpp
        Opcodes/ftsamplebank.cpp
        Opcodes/mixer.cpp
        Opcodes/signalflowgraph.cpp)
    set_source_files_properties(${static_modules_SRCS} PROPERTIES COMPILE_FLAGS -DINIT_STATIC_MODULES)
    list(APPEND libcsound_SRCS ${static_modules_SRCS})
endif()

# Handling New Parser

set(YACC_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Engine/csound_orc.y)
set(YACC_OUT ${CMAKE_CURRENT_BINARY_DIR}/csound_orcparse.c)
set(YACC_OUTH ${CMAKE_CURRENT_BINARY_DIR}/csound_orcparse.h)

set(LEX_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Engine/csound_orc.lex)
set(LEX_OUT ${CMAKE_CURRENT_BINARY_DIR}/csound_orclex.c)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/csound_orclex.c
PROPERTIES COMPILE_FLAGS "-Wno-implicit-fallthrough")
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
    Engine/new_orc_parser.c
    Engine/symbtab.c)

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

#set_source_files_properties(${YACC_SCOOUT} GENERATED)
#set_source_files_properties(${LEX_SCOOUT} GENERATED)
set_source_files_properties(${PRELEX_SCOOUT} GENERATED)
if(MSVC)
  set_source_files_properties(${PRELEX_SCOOUT} PROPERTIES
    COMPILE_FLAGS -DYY_NO_UNISTD_H)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

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

    list(APPEND libcsound_CFLAGS -DPARCS)

else()
    message(STATUS "Not building with multicore support.")
endif()

set(CSOUNDLIB_STATIC "${CSOUNDLIB}-static")

# ADDING HERE TO GRAB LIST OF HEADERS IN CASE OF BUILDING OSX FRAMEWORK
add_subdirectory(include)


if(APPLE)
    string(REGEX REPLACE "([^;]+)(;|$)" "include/\\1\\2" csheaders "${csheaders}")
    file(GLOB H_headers "${CMAKE_CURRENT_SOURCE_DIR}/H/*.h")
    set(libcsound_SRCS
    "${libcsound_SRCS};${csheaders};${H_headers};${CMAKE_CURRENT_BINARY_DIR}/include/float-version.h;${CMAKE_CURRENT_BINARY_DIR}/include/version.h")
endif()

add_library(${CSOUNDLIB} SHARED ${libcsound_SRCS})
set_target_properties(${CSOUNDLIB} PROPERTIES
    SOVERSION ${APIVERSION})

if(APPLE)
    set_target_properties(${CSOUNDLIB} PROPERTIES FRAMEWORK YES)
    set_target_properties(${CSOUNDLIB} PROPERTIES FRAMEWORK_VERSION "${APIVERSION}")
    set_target_properties(${CSOUNDLIB} PROPERTIES PUBLIC_HEADER
    "${csheaders};${CMAKE_CURRENT_BINARY_DIR}/include/float-version.h;${CMAKE_CURRENT_BINARY_DIR}/include/version.h"
    )
endif()


# Utility function to make plugins. All plugin targets should use this as it
# sets up output directory set in top-level CmakeLists.txt
# and adds the appropriate install target
#
# libname - name of library to produce
# srcs - list of src files (must be quoted if a list)
# extralibs (OPTIONAL) - extra libraries to link the plugin to
#
# NB - this was moved here as it needs some VARS defined above
# for setting up the framework
function(make_plugin libname srcs)
    if(APPLE)
        add_library(${libname} SHARED ${srcs})
    else()
        add_library(${libname} MODULE ${srcs})
    endif()

    set(i 2)
    while( ${i} LESS ${ARGC} )
        target_link_libraries(${libname} ${ARGV${i}})
        math(EXPR i "${i}+1")
    endwhile()

    set_target_properties(${libname} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR})

    install(TARGETS ${libname}
        LIBRARY DESTINATION "${PLUGIN_INSTALL_DIR}"
        ARCHIVE DESTINATION "${PLUGIN_INSTALL_DIR}" )

endfunction(make_plugin)

# Add the install target
if(WIN32)
  install(TARGETS ${CSOUNDLIB}
    RUNTIME DESTINATION "${EXECUTABLE_INSTALL_DIR}"
    ARCHIVE DESTINATION "${LIBRARY_INSTALL_DIR}"
    FRAMEWORK DESTINATION "${CS_FRAMEWORK_DEST}")
else()
  install(TARGETS ${CSOUNDLIB}
    LIBRARY DESTINATION "${LIBRARY_INSTALL_DIR}"
    ARCHIVE DESTINATION "${LIBRARY_INSTALL_DIR}"
    FRAMEWORK DESTINATION "${CS_FRAMEWORK_DEST}")
endif()

if(EMSCRIPTEN)
  include_directories(emscripten/deps/libsndfile-1.0.25/src/)
endif()

# TODO include target rather than libs to also bring in transitive dependencies
set(libcsound_LIBS ${LIBSNDFILE_LIBRARY})

if(REQUIRE_PTHREADS AND PTHREAD_LIBRARY)
  list(APPEND libcsound_LIBS ${PTHREAD_LIBRARY})
endif()

if (HAVE_ATOMIC_BUILTIN)
  list(APPEND libcsound_LIBS ${ATOMIC_LIB})
endif()

if(WIN32)
    list(APPEND libcsound_LIBS "${CSOUND_WINDOWS_LIBRARIES}")
    list(APPEND libcsound_LIBS "${LIBSNDFILE_SUPPORT_LIBS}")
endif()

if(USE_CURL AND CURL_FOUND)
  list(APPEND libcsound_LIBS ${CURL_LIBRARIES})
  if(WIN32 AND NOT MSVC)
    # not quite working yet with msys2/mingw64
    #list(APPEND libcsound_CFLAGS -DCURL_STATICLIB -DLIBSSH_STATIC)
    #list(APPEND libcsound_LIBS crypto ssh2 ssl ws2_32 rtmp)
    #find_library(GNUTLS_LIB gnutls)
    #list(APPEND libcsound_LIBS ${GNUTLS_LIB})
  endif()

endif()

# Linux does not have a separate libintl, it is part of libc
set(LIBINTL_AVAIL (LIBINTL_LIBRARY OR LINUX))
check_deps(USE_GETTEXT LIBINTL_AVAIL GETTEXT_MSGFMT_EXECUTABLE)
if(USE_GETTEXT)
    message(STATUS "Using GNU gettext.")
    if(NOT LINUX)
        list(APPEND libcsound_LIBS ${LIBINTL_LIBRARY})
    endif()
    list(APPEND libcsound_CFLAGS -DGNU_GETTEXT)
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

if(LINUX)
    message(STATUS "Building on Linux.")
    add_definitions(-DLINUX -DPIPES -DNO_FLTK_THREADS -D_GNU_SOURCE -DHAVE_SOCKETS)
    list(APPEND libcsound_LIBS ${MATH_LIBRARY} dl)

    find_library(LIBRT_LIBRARY rt)

    if(LIBRT_LIBRARY)
      list(APPEND libcsound_LIBS ${LIBRT_LIBRARY})
      message(STATUS "  ADDING LIBRT LIBRARY: ${LIBRT_LIBRARY}.")
    endif()

    find_library(LIBEXECINFO_LIBRARY execinfo)

    if(LIBEXECINFO_LIBRARY)
       list(APPEND libcsound_LIBS ${LIBEXECINFO_LIBRARY})
       message(STATUS "  ADDING LIBEXECINFO LIBRARY: ${LIBEXECINFO_LIBRARY}.")
    endif()
endif()

if(APPLE AND NOT IOS)
    message(STATUS "Building on OSX")
    add_definitions(-DMACOSX -DPIPES -DNO_FLTK_THREADS -DHAVE_SOCKETS)
    find_library(ACCELERATE_LIBRARY Accelerate)
    find_path(VECLIB_PATH "Accelerate/Accelerate.h")
    include_directories(${VECLIB_PATH})
    list(APPEND libcsound_LIBS ${MATH_LIBRARY} dl ${ACCELERATE_LIBRARY})
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
if(HAVE_STRTOD_L)
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

if(HAVE_WINSOCK_H OR HAVE_SYS_SOCKETS_H)
    list(APPEND libcsound_CFLAGS -DHAVE_SOCKETS)
endif()
if(HAVE_DIRENT_H)
    list(APPEND libcsound_CFLAGS -DHAVE_DIRENT_H)
endif()
if(HAVE_FCNTL_H)
    list(APPEND libcsound_CFLAGS -DHAVE_FCNTL_H)
endif()
if(HAVE_UNISTD_H)
    list(APPEND libcsound_CFLAGS -DHAVE_UNISTD_H)
endif()
if(HAVE_STDINT_H)
    list(APPEND libcsound_CFLAGS -DHAVE_STDINT_H)
endif()
if(HAVE_SYS_TIME_H)
    list(APPEND libcsound_CFLAGS -DHAVE_SYS_TIME_H)
endif()
if(HAVE_SYS_TYPES_H)
    list(APPEND libcsound_CFLAGS -DHAVE_SYS_TYPES_H)
endif()
if(HAVE_TERMIOS_H)
    list(APPEND libcsound_CFLAGS -DHAVE_TERMIOS_H)
endif()
if(HAVE_VALUES_H)
    list(APPEND libcsound_CFLAGS -DHAVE_VALUES_H)
endif()
#if(CMAKE_C_COMPILER MATCHES "gcc")
#    list(APPEND libcsound_CFLAGS -fno-strict-aliasing)
#endif()

if(BIG_ENDIAN)
    list(APPEND libcsound_CFLAGS -DWORDS_BIGENDIAN)
endif()


target_compile_options(${CSOUNDLIB} PRIVATE ${libcsound_CFLAGS})
if(MSVC)
    target_link_libraries(${CSOUNDLIB} PRIVATE SndFile::sndfile ws2_32)
else()
    target_link_libraries(${CSOUNDLIB} PRIVATE ${libcsound_LIBS})
endif()

set_target_properties(${CSOUNDLIB} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR})

if(BUILD_STATIC_LIBRARY)
    add_library(${CSOUNDLIB_STATIC} STATIC ${libcsound_SRCS})
    SET_TARGET_PROPERTIES(${CSOUNDLIB_STATIC} PROPERTIES OUTPUT_NAME ${CSOUNDLIB})
    SET_TARGET_PROPERTIES(${CSOUNDLIB_STATIC} PROPERTIES PREFIX "lib")
    target_compile_options(${CSOUNDLIB_STATIC} PRIVATE ${libcsound_CFLAGS})

    if(MSVC)
        target_link_libraries(${CSOUNDLIB_STATIC} PRIVATE SndFile::sndfile ${CSOUND_WINDOWS_LIBRARIES})
    else()
        target_link_libraries(${CSOUNDLIB_STATIC} ${libcsound_LIBS})
    endif()

    set_target_properties(${CSOUNDLIB_STATIC} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR})

    # Add the install target
    install(TARGETS ${CSOUNDLIB_STATIC}
        LIBRARY DESTINATION "${LIBRARY_INSTALL_DIR}"
        ARCHIVE DESTINATION "${LIBRARY_INSTALL_DIR}")
endif()


add_subdirectory(Opcodes)
add_subdirectory(InOut)
add_subdirectory(Frontends)
add_subdirectory(Java)
add_subdirectory(Python)
add_subdirectory(Bela)
add_subdirectory(util)
add_subdirectory(util1)
add_subdirectory(po)
add_subdirectory(tests/c)
add_subdirectory(tests/commandline)
add_subdirectory(tests/regression)
add_subdirectory(tests/soak)

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
set (CPACK_PACKAGE_VERSION "6.0.0")
set (CPACK_PACKAGE_VERSION_MAJOR "6")
set (CPACK_PACKAGE_VERSION_MINOR "0")
set (CPACK_PACKAGE_VERSION_PATCH "0")
include (CPack)

# Documentation
find_package(Doxygen)
if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.doxygen
          ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc ${DOXYGEN_EXECUTABLE}
            ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
     )
 else()
   message(STATUS "Not building documentation")
endif(DOXYGEN_FOUND)

# install CMake module
install(FILES ${CMAKE_SOURCE_DIR}/cmake/Modules/FindCsound.cmake
        DESTINATION "${CMAKE_INSTALL_PREFIX}/share/cmake/Csound")

# install samples
if(APPLE)
 install(DIRECTORY ${CMAKE_SOURCE_DIR}/samples
          DESTINATION  ${CS_FRAMEWORK_DEST}/${CSOUNDLIB}.framework/Versions/${APIVERSION})
else()
 install(DIRECTORY ${CMAKE_SOURCE_DIR}/samples
          DESTINATION ${CMAKE_INSTALL_PREFIX}/share)
endif()

