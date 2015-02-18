# CUSTOM PROPERTIES TO SET

# GLOBAL

#set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(BUILD_STATIC_LIBRARY ON)
set(USE_OPEN_MP OFF)
set(TCL_VERSION 8.5)
##set(BUILD_CSOUND_AC OFF)
##set(BUILD_CSOUND_AC_PYTHON_INTERFACE OFF)
##set(BUILD_CSOUND_AC_LUA_INTERFACE OFF)

list(APPEND CMAKE_SYSTEM_LIBRARY_PATH "/Users/victor/src/stk-4.4.4/src")
list(APPEND CMAKE_SYSTEM_INCLUDE_PATH "/Users/victor/src/stk-4.4.4/include")

#### NOTE the processor type needs setting
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W -Wall -O3 -mtune=core2")

#include(CheckCCompilerFlag)
#check_c_compiler_flag(-ftree-vectorize HAS_TREE_VECTORISE)
#if (HAS_TREE_VECTORISE)
#    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftree-vectorize")
#endif()

#check_c_compiler_flag(-ffast-math HAS_FAST_MATH)
#if (HAS_FAST_MATH)
#    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffast-math")
#endif()

set(MAX_SDK_ROOT /Users/stevenyi/work/max_msp/MaxSDK-6.1.4)

if(WIN32)
    list(APPEND CMAKE_SYSTEM_INCLUDE_PATH 
	    "c:/work/libsndfile-1_0_17")
    list(APPEND CMAKE_SYSTEM_LIBRARY_PATH
	    "c:/work/libsndfile-1_0_17")

    list(APPEND CMAKE_SYSTEM_LIBRARY_PATH
        "c:/Python25/libs")
    list(APPEND CMAKE_SYSTEM_INCLUDE_PATH
        "c:/Python25/include")

endif()

list(APPEND CMAKE_SYSTEM_LIBRARY_PATH "/usr/local/lib")
list(APPEND CMAKE_SYSTEM_INCLUDE_PATH "/usr/local/include")
list(APPEND CMAKE_SYSTEM_INCLUDE_PATH "/usr/include")
list(APPEND CMAKE_SYSTEM_LIBRARY_PATH "/Users/stevenyi/work/csound/dependencies/stk-4.4.4/src")
list(APPEND CMAKE_SYSTEM_INCLUDE_PATH "/Users/stevenyi/work/csound/dependencies/stk-4.4.4/include")

#add_definitions(-I/usr/local/include -I/usr/include)

set(CMAKE_C_FLAGS "-I/usr/local/include -I/usr/include")
set(CMAKE_CXX_FLAGS "-I/usr/local/include -I/usr/include")
#list(APPEND CMAKE_REQUIRED_DEFINITIONS "-I/usr/include")
#list(APPEND CMAKE_SYSTEM_INCLUDE_PATH "/usr/local/include/luajit-2.0")
list(APPEND CMAKE_SYSTEM_INCLUDE_PATH "/Applications/Pd-extended.app/Contents/Resources/include")
