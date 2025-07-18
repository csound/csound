# Opus_FOUND: if found
# Opus::opus: imported module

include(FindPackageHandleStandardArgs)

find_path(Opuss_INCLUDE_DIR NAMES opus/opus.h)
find_library(Opuss_LIBRARY NAMES opus libopus)

find_package_handle_standard_args(Opuss
	FOUND_VAR Opuss_FOUND
	REQUIRED_VARS Opuss_LIBRARY Opuss_INCLUDE_DIR
)

if (Opuss_FOUND AND NOT TARGET Opuss::opuss)
	add_library(Opuss::opuss UNKNOWN IMPORTED)
	set_target_properties(Opuss::opuss PROPERTIES
		IMPORTED_LOCATION ${Opuss_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${Opuss_INCLUDE_DIR}
	)
endif()

mark_as_advanced(Opuss_LIBRARY Opuss_INCLUDE_DIR)