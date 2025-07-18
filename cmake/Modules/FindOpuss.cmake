# Opus_FOUND: if found
# Opus::opus: imported module

include(FindPackageHandleStandardArgs)

find_path(Opus_INCLUDE_DIR NAMES opus/opus.h)
find_library(Opus_LIBRARY NAMES opus libopus)

find_package_handle_standard_args(Opus
	FOUND_VAR Opus_FOUND
	REQUIRED_VARS Opus_LIBRARY Opus_INCLUDE_DIR
)

if (Opus_FOUND AND NOT TARGET Opus::opus)
	add_library(Opus::opus UNKNOWN IMPORTED)
	set_target_properties(Opus::opus PROPERTIES
		IMPORTED_LOCATION ${Opus_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${Opus_INCLUDE_DIR}
	)
endif()

mark_as_advanced(Opus_LIBRARY Opus_INCLUDE_DIR)