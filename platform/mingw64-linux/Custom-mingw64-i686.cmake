# CUSTOM PROPERTIES TO SET

# GLOBAL

#set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(BUILD_STATIC_LIBRARY ON)
set(TCL_VERSION 8.5)
#set(PYTHON_INCLUDE_DIRS "/usr/include/python2.7")
##set(BUILD_CSOUND_AC OFF)
##set(BUILD_CSOUND_AC_PYTHON_INTERFACE OFF)
##set(BUILD_CSOUND_AC_LUA_INTERFACE OFF)


GET_FILENAME_COMPONENT(MINGW_DEPS_DIR "${CMAKE_CURRENT_BINARY_DIR}/../mingw64" ABSOLUTE)
list(APPEND CMAKE_SYSTEM_LIBRARY_PATH "$MINGW_DEPS_DIR/usr/local/lib")
set(CMAKE_SYSTEM_INCLUDE_PATH "$CMAKE_SYSTEM_INCLUDE_PATH};$MINGW_DEPS_DIR/usr/local/include")

#### NOTE the processor type needs setting
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W -Wall -O3 -mtune=core2 -I${MINGW_DEPS_DIR}/usr/local/include --sysroot=/usr/i686-w64-mingw32")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -O3 -mtune=core2 -fpermissive")
## also to test multicore
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -Wno-missing-field-initializers -Wno-unused-parameter -O3 -mtune=core2 -DJPFF")

