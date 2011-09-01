# CUSTOM PROPERTIES TO SET

# GLOBAL

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_VERBOSE_MAKEFILE ON)

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

