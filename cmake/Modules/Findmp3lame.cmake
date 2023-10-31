# mp3lame_FOUND: whether mp3lame was found
# mp3lame::mp3lame: imported module
# no include dir needed because it is private dependency of SndFile

include(FindPackageHandleStandardArgs)

find_library(mp3lame_LIBRARY NAMES "mp3lame")

find_package_handle_standard_args(mp3lame
	FOUND_VAR mp3lame_FOUND
	REQUIRED_VARS mp3lame_LIBRARY
)

if(mp3lame_FOUND AND NOT TARGET mp3lame::mp3lame)
	add_library(mp3lame::mp3lame UNKNOWN IMPORTED)
	set_target_properties(mp3lame::mp3lame PROPERTIES
		IMPORTED_LOCATION "${mp3lame_LIBRARY}"
	)
endif()

mark_as_advanced(mp3lame_LIBRARY)