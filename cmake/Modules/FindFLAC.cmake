# FLAC_FOUND: if found
# FLAC::flac: imported module

include(FindPackageHandleStandardArgs)

find_path(FLAC_INCLUDE_DIR NAMES FLAC/all.h)
find_library(FLAC_LIBRARY NAMES FLAC libFLAC)

find_package_handle_standard_args(FLAC
	FOUND_VAR FLAC_FOUND
	REQUIRED_VARS FLAC_LIBRARY FLAC_INCLUDE_DIR
)

if (FLAC_FOUND AND NOT TARGET FLAC::flac)
	add_library(FLAC::flac UNKNOWN IMPORTED)
	set_target_properties(FLAC::flac PROPERTIES
		IMPORTED_LOCATION ${FLAC_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${FLAC_INCLUDE_DIR}
	)
endif()

mark_as_advanced(FLAC_LIBRARY FLAC_INCLUDE_DIR)