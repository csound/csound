# CoreMIDI_FOUND: if found
# CoreMIDI::coremidi: imported module

include(FindPackageHandleStandardArgs)

find_library(CoreMIDI_LIBRARY NAMES CoreMIDI)
find_path(CoreMIDI_INCLUDE_DIR NAMES CoreMIDI.h)

find_package_handle_standard_args(CoreMIDI
	FOUND_VAR CoreMIDI_FOUND
	REQUIRED_VARS CoreMIDI_LIBRARY CoreMIDI_INCLUDE_DIR
)

if(CoreMIDI_FOUND AND NOT TARGET CoreMIDI::coremidi)
	add_library(CoreMIDI::coremidi UNKNOWN IMPORTED)
	set_target_properties(CoreMIDI::coremidi PROPERTIES
		IMPORTED_LOCATION ${CoreMIDI_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${CoreMIDI_INCLUDE_DIR}
	)
endif()

mark_as_advanced(CoreMIDI_LIBRARY CoreMIDI_INCLUDE_DIR)
