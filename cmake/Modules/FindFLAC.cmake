# FLAC_FOUND: whether FLAC was found
# FLAC::FLAC: imported module
# no include dir needed because it is private dependency of SndFile

include(FindPackageHandleStandardArgs)
include(CMakeFindDependencyMacro)

find_dependency(Ogg)

find_library(FLAC_LIBRARY NAMES "FLAC")

find_package_handle_standard_args(FLAC
	FOUND_VAR FLAC_FOUND
	REQUIRED_VARS FLAC_LIBRARY
)

if(FLAC_FOUND AND NOT TARGET FLAC::FLAC)
	add_library(FLAC::FLAC UNKNOWN IMPORTED)
	set_target_properties(FLAC::FLAC PROPERTIES
		IMPORTED_LOCATION "${FLAC_LIBRARY}"
	)
	set_property(TARGET FLAC::FLAC
		PROPERTY IMPORTED_LINK_DEPENDENT_LIBRARIES
			m
			Ogg::ogg
	)
endif()

mark_as_advanced(FLAC_LIBRARY)