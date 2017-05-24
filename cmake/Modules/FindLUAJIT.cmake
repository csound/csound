# Try to find the LuaJIT library.
# Once done this will define
#  LUAJIT_FOUND - System has LuaJIT
#  LUAJIT_INCLUDE_DIRS - The LuaJIT include directories
#  LUAJIT_LIBRARIES - The libraries needed to use LuaJIT
#  LUAJIT_DEFINITIONS - Compiler switches required for using LuaJIT
find_package(PkgConfig)
pkg_check_modules(PC_LUAJIT QUIET luajit)
set(LUAJIT_DEFINITIONS ${PC_LUAJIT_CFLAGS_OTHER})
find_path(LUAJIT_INCLUDE_DIR luajit-2.0/luajit.h
          HINTS ${PC_LUAJIT_INCLUDEDIR} ${PC_LUAJIT_INCLUDE_DIRS}
          PATH_SUFFIXES luajit-2.0 )
if(WIN32)
    find_library(LUAJIT_LIBRARY lua51
                 HINTS ${PC_LUAJIT_LIBDIR} ${PC_LUAJIT_LIBRARY_DIRS} )
else()
    find_library(LUAJIT_LIBRARY NAMES luajit lua51
                 HINTS ${PC_LUAJIT_LIBDIR} ${PC_LUAJIT_LIBRARY_DIRS} )
endif()
include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LUAJIT_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LUAJIT DEFAULT_MSG
                                  LUAJIT_LIBRARY LUAJIT_INCLUDE_DIR)
mark_as_advanced(LUAJIT_INCLUDE_DIR LUAJIT_LIBRARY )
set(LUAJIT_LIBRARIES ${LUAJIT_LIBRARY} )
set(LUAJIT_INCLUDE_DIRS ${LUAJIT_INCLUDE_DIR} )

