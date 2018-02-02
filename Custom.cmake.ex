# CUSTOM PROPERTIES TO SET

# GLOBAL
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(BUILD_STATIC_LIBRARY ON)
set(TCL_VERSION 8.5)
set(PYTHON_INCLUDE_DIRS "/usr/include/python2.7")

#### NOTE the processor type needs setting
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W -Wall -mtune=core2")
## also to test multicore
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -Wno-missing-field-initializers -Wno-unused-parameter -mtune=core2 -DJPFF")

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

