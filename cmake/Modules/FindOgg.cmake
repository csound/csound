# Ogg_FOUND: whether Ogg was found
# Ogg::ogg: imported module
# no include dir needed because it is private dependency of SndFile

include(FindPackageHandleStandardArgs)

find_library(Ogg_LIBRARY NAMES "ogg")

find_package_handle_standard_args(Ogg
	FOUND_VAR Ogg_FOUND
	REQUIRED_VARS Ogg_LIBRARY
)

if(Ogg_FOUND AND NOT TARGET Ogg::ogg)
	add_library(Ogg::ogg UNKNOWN IMPORTED)
	set_target_properties(Ogg::ogg PROPERTIES
		IMPORTED_LOCATION ${Ogg_LIBRARY}
	)
endif()

mark_as_advanced(Ogg_LIBRARY)