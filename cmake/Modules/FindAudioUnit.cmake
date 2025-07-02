# AudioUnit_FOUND: if found
# AudioUnit::audiounit: imported module
include(FindPackageHandleStandardArgs)

find_library(AudioUnit_LIBRARY NAMES AudioUnit)
find_path(AudioUnit_INCLUDE_DIR NAMES AudioUnit.h)

find_package_handle_standard_args(AudioUnit
	FOUND_VAR AudioUnit_FOUND
	REQUIRED_VARS AudioUnit_LIBRARY AudioUnit_INCLUDE_DIR
)

if(AudioUnit_FOUND AND NOT TARGET AudioUnit::audiounit)
	add_library(AudioUnit::audiounit UNKNOWN IMPORTED)
	set_target_properties(AudioUnit::audiounit PROPERTIES
		IMPORTED_LOCATION ${AudioUnit_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${AudioUnit_INCLUDE_DIR}
	)
endif()

mark_as_advanced(AudioUnit_LIBRARY AudioUnit_INCLUDE_DIR)
