# FLACS_FOUND: if found
# FLAC::flac: imported module

include(FindPackageHandleStandardArgs)

find_path(FLACS_INCLUDE_DIR NAMES FLAC/all.h)
find_library(FLACS_LIBRARY NAMES FLAC libFLAC)

find_package_handle_standard_args(FLACS
	FOUND_VAR FLACS_FOUND
	REQUIRED_VARS FLACS_LIBRARY FLACS_INCLUDE_DIR
)

if (FLACS_FOUND AND NOT TARGET FLACS::flacs)
	add_library(FLACS::flacs UNKNOWN IMPORTED)
	set_target_properties(FLACS::flacs PROPERTIES
		IMPORTED_LOCATION ${FLACS_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${FLACS_INCLUDE_DIR}
	)
endif()

mark_as_advanced(FLACS_LIBRARY FLACS_INCLUDE_DIR)