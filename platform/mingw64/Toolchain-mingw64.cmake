# the name of the target operating system
#SET(CMAKE_SYSTEM_NAME Windows)

# which compilers to use for C and C++
#SET(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
#SET(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
#SET(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

#GET_FILENAME_COMPONENT(MINGW_DEPS_DIR "${CMAKE_CURRENT_BINARY_DIR}/../mingw64" ABSOLUTE)
#message(STATUS ">>> ${MINGW_DEPS_DIR}")
# here is the target environment located
#SET(CMAKE_FIND_ROOT_PATH  /usr/x86_64-w64-mingw32 ${MINGW_DEPS_DIR} ${MINGW_DEPS_DIR}/usr/local)
#SET(CMAKE_FIND_ROOT_PATH  /mingw64 /mingw64/x86_64-w64-mingw32 /usr)

#SET(CMAKE_SYSTEM_PREFIX_PATH /mingw64/x85_64-w64-mingw32)

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
#set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
#set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)


# http://www.cmake.org/Wiki/CMake_Cross_Compiling#The_toolchain_file
# http://bulletphysics.org/Bullet/phpBB3/viewtopic.php?t=8959
# http://stackoverflow.com/questions/19754316/cross-compiling-opencv-with-mingw-using-cmakein-linux-for-windows

# this one is important
#SET(CMAKE_SYSTEM_NAME Windows)
##this one not so much
##SET(CMAKE_SYSTEM_VERSION 1)

## specify the cross compiler
#SET(PREFIX x86_64-w64-mingw32)
##SET(CMAKE_MAKE_PROGRAM mingw32-make)
#SET(CMAKE_C_COMPILER   ${PREFIX}-gcc)
#SET(CMAKE_CXX_COMPILER ${PREFIX}-g++)
#SET(CMAKE_AR ${PREFIX}-gcc-ar)
#SET(CMAKE_NM ${PREFIX}-gcc-nm)
#SET(CMAKE_RC_COMPILER  windres)

## specify the cross linker
#SET(CMAKE_RANLIB ${PREFIX}-gcc-ranlib)

# where is the target environment
#SET(CMAKE_FIND_ROOT_PATH / /mingw64)
