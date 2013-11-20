# CUSTOM PROPERTIES TO SET

# GLOBAL

#set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(BUILD_STATIC_LIBRARY ON)
set(USE_OPEN_MP OFF)
set(TCL_VERSION 8.5)
#set(PYTHON_INCLUDE_DIRS "/usr/include/python2.7")
##set(BUILD_CSOUND_AC OFF)
##set(BUILD_CSOUND_AC_PYTHON_INTERFACE OFF)
##set(BUILD_CSOUND_AC_LUA_INTERFACE OFF)

#### NOTE the processor type needs setting
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W -Wall -O3 -mtune=core2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -O3 -mtune=core2 -fpermissive")
## also to test multicore
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -Wno-missing-field-initializers -Wno-unused-parameter -O3 -mtune=core2 -DJPFF")


