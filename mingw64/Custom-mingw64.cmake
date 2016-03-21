message("Custom-mingw64.cmake is being included.")
set(BUILD_CSOUND_VST ON)
set(BUILD_PD_CLASS OFF)
set(BUILD_STATIC_LIBRARY ON)
set(BUILD_TESTS OFF)
set(BUILD_VST4CS ON)
set(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(NEED_PORTTIME OFF)
set(TCL_VERSION 8.5)
set(USE_OPEN_MP OFF)

#Turning off use of CURL until a static library can be used
set(USE_CURL OFF)
