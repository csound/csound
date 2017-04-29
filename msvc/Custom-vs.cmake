# CUSTOM PROPERTIES TO SET
# GLOBAL

set(CMAKE_VERBOSE_MAKEFILE ON)
set(BUILD_STATIC_LIBRARY ON)
set(USE_OPEN_MP OFF)
set(TCL_VERSION 8.5)

# Disable the following warnings in msvc
# C4244 loss of data in conversion
# C4267 loss of data in conversion
# C4005 macro refinitions
# C4996 unsafe functions
# C4047 levels of indirection difference (int and void*)
# C4090 different const qualifiers
# C4477 format string type differences
set(CMAKE_C_FLAGS "${CMAKE_CX_FLAGS} /wd4244 /wd4267 /wd4005 /wd4996 /wd4047 /wd4090 /wd4477")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244 /wd4267 /wd4005 /wd4996 /wd4047 /wd4090 /wd4477")

set(BUILD_PD_CLASS OFF)

# FIXME this is supposedly a bad thing, figure out something else maybe
list(APPEND CMAKE_SYSTEM_INCLUDE_PATH 
  "${CMAKE_CURRENT_BINARY_DIR}/../deps/include")
list(APPEND CMAKE_SYSTEM_LIBRARY_PATH
  "${CMAKE_CURRENT_BINARY_DIR}/../deps/lib")


