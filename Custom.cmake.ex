# CUSTOM PROPERTIES TO SET

# GLOBAL

#set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(BUILD_STATIC_LIBRARY ON)
set(USE_OPEN_MP OFF)
set(TCL_VERSION 8.5)
set(PYTHON_INCLUDE_DIRS "/usr/include/python2.7")
##set(BUILD_CSOUND_AC OFF)
##set(BUILD_CSOUND_AC_PYTHON_INTERFACE OFF)
##set(BUILD_CSOUND_AC_LUA_INTERFACE OFF)

#### NOTE the processor type needs setting
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W -Wall -mtune=core2")
## also to test multicore
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -Wno-missing-field-initializers -Wno-unused-parameter -mtune=core2 -DJPFF")

include(CheckCCompilerFlag)
check_c_compiler_flag(-ftree-vectorize HAS_TREE_VECTORISE)
if (HAS_TREE_VECTORISE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftree-vectorize")
endif()

check_c_compiler_flag(-ffast-math HAS_FAST_MATH)
if (HAS_FAST_MATH)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffast-math")
endif()

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

