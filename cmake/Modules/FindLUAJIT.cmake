# Try to find the LuaJIT library.
# Once done this will define:
#  LUAJIT_FOUND - The system has the LuaJIT library.
#  LUAJIT_INCLUDE_DIRS - The LuaJIT include directories.
#  LUAJIT_LIBRARIES - The libraries needed to use LuaJIT.

find_path(LUAJIT_INCLUDE_DIR luajit-2.0/lua.h)
find_library(LUAJIT_LIBRARY NAMES lua51)

set(LUAJIT_INCLUDE_DIRS ${LUAJIT_INCLUDE_DIR} )
set(LUAJIT_LIBRARIES ${LUAJIT_LIBRARY} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LUAJIT_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LUAJIT DEFAULT_MSG
                                  LUAJIT_LIBRARY LUAJIT_INCLUDE_DIR)

mark_as_advanced(LUAJIT_INCLUDE_DIR LUAJIT_LIBRARY )
