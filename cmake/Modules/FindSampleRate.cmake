# SampleRate_FOUND: if found
# SampleRate::samplerate: imported module

include(FindPackageHandleStandardArgs)

find_library(SampleRate_LIBRARY NAMES samplerate libsamplerate)
find_path(SampleRate_INCLUDE_DIR NAMES samplerate.h)

find_package_handle_standard_args(SampleRate
	FOUND_VAR SampleRate_FOUND
	REQUIRED_VARS SampleRate_LIBRARY SampleRate_INCLUDE_DIR
)

if(SampleRate_FOUND AND NOT TARGET SampleRate::samplerate)
	add_library(SampleRate::samplerate UNKNOWN IMPORTED)
	set_target_properties(SampleRate::samplerate PROPERTIES
		IMPORTED_LOCATION ${SampleRate_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${SampleRate_INCLUDE_DIR}
	)
endif()

mark_as_advanced(SampleRate_LIBRARY SampleRate_INCLUDE_DIR)
