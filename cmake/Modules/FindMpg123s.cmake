#  Mpg123_FOUND: if found
#  Mpg123::mpg123: imported module

include(FindPackageHandleStandardArgs)

find_path(Mpg123_INCLUDE_DIR NAMES  mpg123.h)
find_library(Mpg123_LIBRARY NAMES mpg123 libmpg123)

find_package_handle_standard_args(Mpg123
	FOUND_VAR  Mpg123_FOUND
	REQUIRED_VARS  Mpg123_LIBRARY  Mpg123_INCLUDE_DIR
)

if (Mpg123_FOUND AND NOT TARGET Mpg123::mpg123)
	add_library(Mpg123::mpg123 UNKNOWN IMPORTED)
	set_target_properties(Mpg123::mpg123 PROPERTIES
		IMPORTED_LOCATION ${Mpg123_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${Mpg123_INCLUDE_DIR}
	)
endif()

mark_as_advanced(Mpg123_LIBRARY Mpg123_INCLUDE_DIR)