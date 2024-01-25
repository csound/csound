# Jack_FOUND: if found
# Jack_STATIC: if static
# Jack::jack: imported module

include(FindPackageHandleStandardArgs)

find_library(Jack_LIBRARY NAMES jack)
find_path(Jack_INCLUDE_DIR NAMES jack/jack.h)

find_package_handle_standard_args(Jack
	FOUND_VAR Jack_FOUND
	REQUIRED_VARS Jack_LIBRARY Jack_INCLUDE_DIR
)

if(Jack_FOUND AND NOT TARGET Jack::jack)
	add_library(Jack::jack UNKNOWN IMPORTED)
	set_target_properties(Jack::jack PROPERTIES
		IMPORTED_LOCATION ${Jack_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${Jack_INCLUDE_DIR}
	)
endif()

mark_as_advanced(Jack_LIBRARY Jack_INCLUDE_DIR)