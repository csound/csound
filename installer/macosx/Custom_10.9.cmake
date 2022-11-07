# CUSTOM PROPERTIES TO SET

# GLOBAL

#set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_OSX_ARCHITECTURES  "arm64;x86_64")
set(CMAKE_VERBOSE_MAKEFILE ON)
#set(BUILD_JACK_OPCODES OFF)
set(BUILD_STATIC_LIBRARY ON)
set(USE_OPEN_MP OFF)
set(TCL_VERSION 8.5)
#set(PYTHON_LIBRARY='/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib')
#set(PYTHON_INCLUDE='/Library/Frameworks/Python.framework/Headers')
set(JNI_INCLUDE_DIRS "/Library/Java/JavaVirtualMachines/jdk1.7.0_45.jdk/Contents/Home/include")

set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -stdlib=libc++ -lc++abi")


#list(APPEND CMAKE_SYSTEM_LIBRARY_PATH "/Users/victor/src/stk-4.4.4/src")
#list(APPEND CMAKE_SYSTEM_INCLUDE_PATH "/Users/victor/src/stk-4.4.4/include")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wsign-compare -std=c++11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wsign-compare -Wmissing-braces")

if(${CMAKE_BUILD_TYPE} MATCHES "Release")
include(CheckCCompilerFlag)
check_c_compiler_flag(-ftree-vectorize HAS_TREE_VECTORISE)
if (HAS_TREE_VECTORISE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftree-vectorize")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftree-vectorize")
endif()

check_c_compiler_flag(-ffast-math HAS_FAST_MATH)
if (HAS_FAST_MATH)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffast-math")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
endif()
endif()
