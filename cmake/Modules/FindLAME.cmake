# - Find LAME
# Find the native LAME includes and library
#
#  LAME_INCLUDE_DIR - where to find lame.h
#  LAME_LIBRARY - lame library
#  LAME_FOUND       - lame found

if (LAME_INCLUDE_DIR AND LAME_LIBRARY)
  # Already in cache, be silent
  set (LAME_FIND_QUIETLY TRUE)
endif ()

find_path (LAME_INCLUDE_DIR lame/lame.h HINTS /usr/local/include)
find_library (LAME_LIBRARY NAMES mp3lame libmp3lame HINTS /usr/local/lib)
mark_as_advanced (LAME_LIBRARIES LAME_INCLUDE_DIR)

# handle the QUIETLY and REQUIRED arguments and set LAME_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LAME DEFAULT_MSG LAME_LIBRARY LAME_INCLUDE_DIR)
