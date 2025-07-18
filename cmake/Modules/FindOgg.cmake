# OGG_FOUND: if found
# Ogg::ogg: imported module

include(FindPackageHandleStandardArgs)

find_path(Ogg_INCLUDE_DIR NAMES ogg/ogg.h)
find_library(Ogg_LIBRARY NAMES ogg libogg.0 libogg)

find_package_handle_standard_args(Ogg
	FOUND_VAR Ogg_FOUND
	REQUIRED_VARS Ogg_LIBRARY Ogg_INCLUDE_DIR
)

if (Ogg_FOUND AND NOT TARGET Ogg::ogg)
	add_library(Ogg::ogg UNKNOWN IMPORTED)
	set_target_properties(Ogg::ogg PROPERTIES
		IMPORTED_LOCATION ${Ogg_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${Ogg_INCLUDE_DIR}
	)
endif()

mark_as_advanced(Ogg_LIBRARY Ogg_INCLUDE_DIR)