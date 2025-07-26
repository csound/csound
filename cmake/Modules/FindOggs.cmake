# OGG_FOUND: if found
# Oggs::oggs: imported module

include(FindPackageHandleStandardArgs)

find_path(Oggs_INCLUDE_DIR NAMES ogg/ogg.h)
find_library(Oggs_LIBRARY NAMES ogg libogg.0 libogg)

find_package_handle_standard_args(Oggs
	FOUND_VAR Oggs_FOUND
	REQUIRED_VARS Oggs_LIBRARY Oggs_INCLUDE_DIR
)

if (Oggs_FOUND AND NOT TARGET Oggs::oggs)
	add_library(Oggs::oggs UNKNOWN IMPORTED)
	set_target_properties(Oggs::oggs PROPERTIES
		IMPORTED_LOCATION ${Oggs_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${Oggs_INCLUDE_DIR}
	)
endif()

mark_as_advanced(Oggs_LIBRARY Oggs_INCLUDE_DIR)