#  Vorbisenc_FOUND: if found
#  Vorbisenc::vorbisenc: imported module

include(FindPackageHandleStandardArgs)

find_path(Vorbisencs_INCLUDE_DIR NAMES  vorbis/vorbisenc.h)
find_library(Vorbisencs_LIBRARY NAMES vorbisenc libvorbisenc)

find_package_handle_standard_args(Vorbisencs
	FOUND_VAR  Vorbisencs_FOUND
	REQUIRED_VARS  Vorbisencs_LIBRARY  Vorbisencs_INCLUDE_DIR
)

if ( Vorbisencs_FOUND AND NOT TARGET  Vorbisencs::vorbisencs)
	add_library(Vorbisencs::vorbisencs UNKNOWN IMPORTED)
	set_target_properties(Vorbisencs::vorbisencs PROPERTIES
		IMPORTED_LOCATION ${Vorbisencs_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${Vorbisencs_INCLUDE_DIR}
	)
endif()

mark_as_advanced(Vorbisenc_LIBRARY Vorbisencs_INCLUDE_DIR)