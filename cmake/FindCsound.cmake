# FindCsound.cmake
#
# Try to find the Csound library.
# Once done this will define:
#
#  CSOUND_FOUND        - System has the Csound library
#  CSOUND_INCLUDE_DIRS - The Csound include directories
#  CSOUND_LIBRARIES    - The libraries needed to use Csound
#  CSOUND_VERSION      - The version of the Csound library found
#
# The following hints are accepted:
#  CSOUND_INCLUDE_DIR_HINT    - Path hint for the include directory
#  CSOUND_LIBRARY_DIR_HINT    - Path hint for the library directory
#

if(APPLE)
    find_path(CSOUND_INCLUDE_DIR csound.h
        HINTS
            "$ENV{HOME}/Library/Frameworks/CsoundLib64.framework/Headers"
            "$ENV{HOME}/Library/Frameworks/CsoundLib.framework/Headers"
            /Library/Frameworks/CsoundLib64.framework/Headers
            /Library/Frameworks/CsoundLib.framework/Headers
            ${CSOUND_INCLUDE_DIR_HINT}
    )
else()
    find_path(CSOUND_INCLUDE_DIR csound.h
        PATH_SUFFIXES csound
        HINTS ${CSOUND_INCLUDE_DIR_HINT}
    )
endif()

if(APPLE)
    find_library(CSOUND_LIBRARY
        NAMES CsoundLib64 CsoundLib
        PATHS
            "$ENV{HOME}/Library/Frameworks"
            /Library/Frameworks
            ${CSOUND_LIBRARY_DIR_HINT}
    )
else()
    find_library(CSOUND_LIBRARY
        NAMES csound64 csound
        HINTS ${CSOUND_LIBRARY_DIR_HINT}
    )
endif()

# Extract version information from version.h if available
if(CSOUND_INCLUDE_DIR AND EXISTS "${CSOUND_INCLUDE_DIR}/version.h")
    file(STRINGS "${CSOUND_INCLUDE_DIR}/version.h" _csound_version_major
        REGEX "^#define[ \t]+CS_VERSION[ \t]+\\(([0-9]+)\\)")
    file(STRINGS "${CSOUND_INCLUDE_DIR}/version.h" _csound_version_minor
        REGEX "^#define[ \t]+CS_SUBVER[ \t]+\\(([0-9]+)\\)")
    file(STRINGS "${CSOUND_INCLUDE_DIR}/version.h" _csound_version_patch
        REGEX "^#define[ \t]+CS_PATCHLEVEL[ \t]+\\(([0-9]+)\\)")

    string(REGEX REPLACE "^#define[ \t]+CS_VERSION[ \t]+\\(([0-9]+)\\)" "\\1"
        _csound_major "${_csound_version_major}")
    string(REGEX REPLACE "^#define[ \t]+CS_SUBVER[ \t]+\\(([0-9]+)\\)" "\\1"
        _csound_minor "${_csound_version_minor}")
    string(REGEX REPLACE "^#define[ \t]+CS_PATCHLEVEL[ \t]+\\(([0-9]+)\\)" "\\1"
        _csound_patch "${_csound_version_patch}")

    if(_csound_major AND _csound_minor AND _csound_patch)
        set(CSOUND_VERSION "${_csound_major}.${_csound_minor}.${_csound_patch}")
    elseif(_csound_major AND _csound_minor)
        set(CSOUND_VERSION "${_csound_major}.${_csound_minor}")
    endif()

    unset(_csound_version_major)
    unset(_csound_version_minor)
    unset(_csound_version_patch)
    unset(_csound_major)
    unset(_csound_minor)
    unset(_csound_patch)
endif()

include(FindPackageHandleStandardArgs)

# Handle the QUIETLY and REQUIRED arguments and set CSOUND_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Csound
    REQUIRED_VARS CSOUND_LIBRARY CSOUND_INCLUDE_DIR
    VERSION_VAR CSOUND_VERSION
)

mark_as_advanced(CSOUND_INCLUDE_DIR CSOUND_LIBRARY)

set(CSOUND_INCLUDE_DIRS ${CSOUND_INCLUDE_DIR})
set(CSOUND_LIBRARIES ${CSOUND_LIBRARY})

# Create imported target Csound::Csound if not already defined
if(CSOUND_FOUND AND NOT TARGET Csound::Csound)
    add_library(Csound::Csound UNKNOWN IMPORTED)
    set_target_properties(Csound::Csound PROPERTIES
        IMPORTED_LOCATION "${CSOUND_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${CSOUND_INCLUDE_DIR}"
    )
endif()
