# PortMidi_FOUND: if found
# PortMidi::portmidi: imported module

include(FindPackageHandleStandardArgs)

find_library(PortMidi_LIBRARY NAMES portmidi portmidi_s)
find_path(PortMidi_INCLUDE_DIR NAMES portmidi.h)

find_package_handle_standard_args(PortMidi
	FOUND_VAR PortMidi_FOUND
	REQUIRED_VARS PortMidi_LIBRARY PortMidi_INCLUDE_DIR
)

if(PortMidi_FOUND AND NOT TARGET PortMidi::portmidi)
	add_library(PortMidi::portmidi UNKNOWN IMPORTED)
	set_target_properties(PortMidi::portmidi PROPERTIES
		IMPORTED_LOCATION ${PortMidi_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${PortMidi_INCLUDE_DIR}
	)
endif()

mark_as_advanced(PortMidi_LIBRARY PortMidi_INCLUDE_DIR)