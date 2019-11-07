# Try to find the Csound library.
# Once done this will define:
#  CSOUND_FOUND - System has the Csound library
#  CSOUND_INCLUDE_DIRS - The Csound include directories.
#  CSOUND_LIBRARIES - The libraries needed to use the Csound library.

if(NOT APPLE)
  find_path(CSOUND_INCLUDE_DIR csound.h PATH_SUFFIXES csound)
endif()

if(NOT APPLE)
  find_library(CSOUND_LIBRARY NAMES csound64 csound)
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set CSOUND_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(CSOUND
                                  CSOUND_LIBRARY CSOUND_INCLUDE_DIR)
mark_as_advanced(CSOUND_INCLUDE_DIR CSOUND_LIBRARY)

if(NOT APPLE)
  set(CSOUND_INCLUDE_DIRS ${CSOUND_INCLUDE_DIR})
  set(CSOUND_LIBRARIES ${CSOUND_LIBRARY} )
endif()
