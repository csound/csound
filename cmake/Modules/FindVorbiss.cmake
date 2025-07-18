# Vorbis_FOUND: if found
# Vorbis::vorbis: imported module

include(FindPackageHandleStandardArgs)

find_path(Vorbis_INCLUDE_DIR NAMES Vorbis/codec.h)
find_library(Vorbis_LIBRARY NAMES vorbis libvorbis)

find_package_handle_standard_args(Vorbis
	FOUND_VAR Vorbis_FOUND
	REQUIRED_VARS Vorbis_LIBRARY Vorbis_INCLUDE_DIR
)

if (Vorbis_FOUND AND NOT TARGET Vorbis::vorbis)
	add_library(Vorbis::vorbis UNKNOWN IMPORTED)
	set_target_properties(Vorbis::vorbis PROPERTIES
		IMPORTED_LOCATION ${Vorbis_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${Vorbis_INCLUDE_DIR}
	)
endif()

mark_as_advanced(Vorbis_LIBRARY Vorbis_INCLUDE_DIR)