# Vorbis_FOUND: if found
# Vorbis::vorbis: imported module

include(FindPackageHandleStandardArgs)

find_path(Vorbiss_INCLUDE_DIR NAMES Vorbis/codec.h)
find_library(Vorbiss_LIBRARY NAMES vorbis libvorbis)

find_package_handle_standard_args(Vorbiss
	FOUND_VAR Vorbiss_FOUND
	REQUIRED_VARS Vorbiss_LIBRARY Vorbiss_INCLUDE_DIR
)

if (Vorbiss_FOUND AND NOT TARGET Vorbiss::vorbiss)
	add_library(Vorbiss::vorbiss UNKNOWN IMPORTED)
	set_target_properties(Vorbiss::vorbiss PROPERTIES
		IMPORTED_LOCATION ${Vorbiss_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${Vorbiss_INCLUDE_DIR}
	)
endif()

mark_as_advanced(Vorbiss_LIBRARY Vorbiss_INCLUDE_DIR)