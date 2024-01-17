# PortMidi_FOUND: if found
# PortMidi::portmidi: imported module

include(FindPackageHandleStandardArgs)

find_path(PortMidi_INCLUDE_DIR
    NAMES
		portmidi.h
    PATHS
		/usr/include
		/usr/local/include
		/opt/local/include
		/sw/include
)

find_library(PortMidi_LIBRARY
    NAMES
		portmidi
		portmidi_s
    PATHS
		/usr/lib
		/usr/local/lib
		/opt/local/lib
		/sw/lib
)

find_package_handle_standard_args(PortMidi
	FOUND_VAR PortMidi_FOUND
	REQUIRED_VARS PortMidi_LIBRARY PortMidi_INCLUDE_DIR
)

if(PortMidi_FOUND)
	add_library(PortMidi::portmidi UNKNOWN IMPORTED)
	set_target_properties(PortMidi::portmidi PROPERTIES
		IMPORTED_LOCATION ${PortMidi_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${PortMidi_INCLUDE_DIR}
	)
endif()
