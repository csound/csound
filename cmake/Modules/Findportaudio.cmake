# portaudio_FOUND: if found
# portaudio_static: imported module

include(FindPackageHandleStandardArgs)

find_path(portaudio_INCLUDE_DIR
	NAMES
		portaudio.h
	PATHS
		/usr/include
		/usr/local/include
		/opt/local/include
		/sw/include
)

find_library(portaudio_LIBRARY
	NAMES
		portaudio portaudio_x64
	PATHS
		/usr/lib
		/usr/local/lib
		/opt/local/lib
		/sw/lib
)

find_package_handle_standard_args(portaudio
	FOUND_VAR portaudio_FOUND
	REQUIRED_VARS portaudio_LIBRARY portaudio_INCLUDE_DIR
)

if(portaudio_FOUND AND NOT TARGET portaudio_static)
	add_library(portaudio_static UNKNOWN IMPORTED)
	set_target_properties(portaudio_static PROPERTIES
		IMPORTED_LOCATION ${portaudio_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${portaudio_INCLUDE_DIR}
	)
endif()

mark_as_advanced(
	portaudio_LIBRARY
	portaudio_INCLUDE_DIR
)
