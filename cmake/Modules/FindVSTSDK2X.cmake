# Try to find the VSTSDK2X library.
# Once done this will define:
#  VSTSDK2X_FOUND - The system has the VST SDK.
#  VSTSDK2X_DIR - The VST SDK root directory.
#  VSTSDK2X_INCLUDE_DIRS - The VST SDK include directories.

find_path(VSTSDK2X_INCLUDE_DIR pluginterfaces/vst2.x/aeffect.h HINTS ${VST_SDK2_HOME} PATH_SUFFIXES vstsdk2.4)

set(VSTSDK2X_DIR ${VSTSDK2X_INCLUDE_DIR})
set(VSTSDK2X_INCLUDE_DIRS ${VSTSDK2X_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set VSTSDK2X_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(VSTSDK2X DEFAULT_MSG
                                  VSTSDK2X_INCLUDE_DIR)

mark_as_advanced(VSTSDK2X_INCLUDE_DIR )
