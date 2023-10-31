# Vorbis_FOUND: whether Vorbis was found
# Vorbis::vorbis: imported module
# Vorbis::vorbisenv: imported module
# no include dir needed because it is private dependency of SndFile

include(FindPackageHandleStandardArgs)
include(CMakeFindDependencyMacro)

find_dependency(Ogg)

find_library(Vorbis_vorbis_LIBRARY NAMES "vorbis")
find_library(Vorbis_vorbisenc_LIBRARY NAMES "vorbisenc")

find_package_handle_standard_args(Vorbis
	FOUND_VAR Vorbis_FOUND
	REQUIRED_VARS
        Vorbis_vorbis_LIBRARY
        Vorbis_vorbisenc_LIBRARY
)

if(Vorbis_FOUND AND NOT TARGET Vorbis::vorbis)
	add_library(Vorbis::vorbis UNKNOWN IMPORTED)
	set_target_properties(Vorbis::vorbis PROPERTIES
		IMPORTED_LOCATION ${Vorbis_vorbis_LIBRARY}
	)
	set_property(TARGET Vorbis::vorbis
		PROPERTY IMPORTED_LINK_DEPENDENT_LIBRARIES
			m
			Ogg::ogg
	)
endif()

if(Vorbis_FOUND AND NOT TARGET Vorbis::vorbisenc)
	add_library(Vorbis::vorbisenc UNKNOWN IMPORTED)
	set_target_properties(Vorbis::vorbisenc PROPERTIES
		IMPORTED_LOCATION ${Vorbis_vorbisenc_LIBRARY}
        IMPORTED_LINK_DEPENDENT_LIBRARIES Vorbis::vorbis
	)
endif()

mark_as_advanced(
	Vorbis_vorbis_LIBRARY
    Vorbis_vorbisenc_LIBRARY
)