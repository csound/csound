# CoreAudio_FOUND: if found
# CoreAudio::coreaudio: imported module
include(FindPackageHandleStandardArgs)

find_library(CoreAudio_LIBRARY NAMES CoreAudio)
find_path(CoreAudio_INCLUDE_DIR NAMES AudioHardware.h)

find_package_handle_standard_args(CoreAudio
	FOUND_VAR CoreAudio_FOUND
	REQUIRED_VARS CoreAudio_LIBRARY CoreAudio_INCLUDE_DIR
)

if(CoreAudio_FOUND AND NOT TARGET CoreAudio::coreaudio)
	add_library(CoreAudio::coreaudio UNKNOWN IMPORTED)
	set_target_properties(CoreAudio::coreaudio PROPERTIES
		IMPORTED_LOCATION ${CoreAudio_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${CoreAudio_INCLUDE_DIR}
	)
endif()

mark_as_advanced(CoreAudio_LIBRARY CoreAudio_INCLUDE_DIR)
