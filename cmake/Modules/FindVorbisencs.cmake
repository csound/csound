#  Vorbisenc_FOUND: if found
#  Vorbisenc::vorbisenc: imported module

include(FindPackageHandleStandardArgs)

find_path(Vorbisenc_INCLUDE_DIR NAMES  vorbis/vorbisenc.h)
find_library(Vorbisenc_LIBRARY NAMES vorbisenc libvorbisenc)

find_package_handle_standard_args(Vorbisenc
	FOUND_VAR  Vorbisenc_FOUND
	REQUIRED_VARS  Vorbisenc_LIBRARY  Vorbisenc_INCLUDE_DIR
)

if ( Vorbisenc_FOUND AND NOT TARGET  Vorbisenc::vorbisenc)
	add_library(Vorbisenc::vorbisenc UNKNOWN IMPORTED)
	set_target_properties(Vorbisenc::vorbisenc PROPERTIES
		IMPORTED_LOCATION ${Vorbisenc_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${Vorbisenc_INCLUDE_DIR}
	)
endif()

mark_as_advanced(Vorbisenc_LIBRARY Vorbisenc_INCLUDE_DIR)