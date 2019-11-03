# Try to find the PortSMF library.
# Once done this will define:
#  PORTSMF_FOUND - The system has the PortSMF library.
#  PORTSMF_INCLUDE_DIRS - The PortSMF include directories.
#  PORTSMF_LIBRARIES - The libraries needed to use MusicXML.

find_path(PORTSMF_INCLUDE_DIR allegro.h PATH_SUFFIXES portsmf)
find_library(PORTSMF_LIBRARY NAMES portSMF)

set(PORTSMF_INCLUDE_DIRS ${PORTSMF_INCLUDE_DIR} )
set(PORTSMF_LIBRARIES ${PORTSMF_LIBRARY} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set PORTSMF_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(PORTSMF DEFAULT_MSG
                                  PORTSMF_LIBRARY PORTSMF_INCLUDE_DIR)

mark_as_advanced(PORTSMF_INCLUDE_DIR PORTSMF_LIBRARY )
