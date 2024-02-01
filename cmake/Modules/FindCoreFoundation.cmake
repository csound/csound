# CoreFoundation_FOUND: if found
# CoreFoundation::corefoundation: imported module
include(FindPackageHandleStandardArgs)

find_library(CoreFoundation_LIBRARY NAMES CoreFoundation)
find_path(CoreFoundation_INCLUDE_DIR NAMES CoreFoundation.h)

find_package_handle_standard_args(CoreFoundation
	FOUND_VAR CoreFoundation_FOUND
	REQUIRED_VARS CoreFoundation_LIBRARY CoreFoundation_INCLUDE_DIR
)

if(CoreFoundation_FOUND AND NOT TARGET CoreFoundation::corefoundation)
	add_library(CoreFoundation::corefoundation UNKNOWN IMPORTED)
	set_target_properties(CoreFoundation::corefoundation PROPERTIES
		IMPORTED_LOCATION ${CoreFoundation_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${CoreFoundation_INCLUDE_DIR}
	)
endif()

mark_as_advanced(CoreFoundation_LIBRARY CoreFoundation_INCLUDE_DIR)
