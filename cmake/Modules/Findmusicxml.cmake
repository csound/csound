# Try to find the MusicXML library.
# Once done this will define:
#  MUSICXML_FOUND - The system has the MusicXML library.
#  MUSICXML_INCLUDE_DIRS - The MusicXML include directories.
#  MUSICXML_LIBRARIES - The libraries needed to use MusicXML.

find_path(MUSICXML_INCLUDE_DIR libmusicxml.h PATH_SUFFIXES libmusicxml)
find_library(MUSICXML_LIBRARY NAMES libmusicxml2.dll musicxml2 libmusicxml2.so)

set(MUSICXML_INCLUDE_DIRS ${MUSICXML_INCLUDE_DIR} )
set(MUSICXML_LIBRARIES ${MUSICXML_LIBRARY} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set MUSICXML_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(MUSICXML DEFAULT_MSG
                                  MUSICXML_LIBRARY MUSICXML_INCLUDE_DIR)

mark_as_advanced(MUSICXML_INCLUDE_DIR MUSICXML_LIBRARY )
