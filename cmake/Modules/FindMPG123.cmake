# MPG123_FOUND: whether MPG123 was found
# MPG123::libmpg123: imported module
# no include dir needed because it is private dependency of SndFile

include(FindPackageHandleStandardArgs)

find_library(MPG123_LIBRARY NAMES "mpg123")

find_package_handle_standard_args(MPG123
	FOUND_VAR MPG123_FOUND
	REQUIRED_VARS MPG123_LIBRARY
)

if(MPG123_FOUND AND NOT TARGET MPG123::libmpg123)
	add_library(MPG123::libmpg123 UNKNOWN IMPORTED)
	set_target_properties(MPG123::libmpg123 PROPERTIES
		IMPORTED_LOCATION "${MPG123_LIBRARY}"
	)
endif()

mark_as_advanced(MPG123_LIBRARY)