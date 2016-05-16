# Try to find the Liblo library.
# Once done this will define:
#  LIBLO_FOUND - System has the OSC library.
#  LIBLO_INCLUDE_DIRS - The OSC include directories.
#  LIBLO_LIBRARIES - The libraries needed to use the OSC library.

find_path(LIBLO_INCLUDE_DIR lo.h PATH_SUFFIXES lo)

if(LINUX)
find_library(LIBLO_LIBRARY NAMES lo)
else()
find_library(LIBLO_LIBRARY NAMES liblo.a lo)
endif()

set(LIBLO_INCLUDE_DIRS ${LIBLO_INCLUDE_DIR})
set(LIBLO_LIBRARIES ${LIBLO_LIBRARY} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBLO_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(DEFAULT_MSG
                                  LIBLO_LIBRARY LIBLO_INCLUDE_DIR)

mark_as_advanced(LIBLO_INCLUDE_DIR LIBLO_LIBRARY )
