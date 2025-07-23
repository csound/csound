#  Mpg123_FOUND: if found
#  Mpg123::mpg123: imported module

include(FindPackageHandleStandardArgs)

find_path(Mpg123s_INCLUDE_DIR NAMES  mpg123.h)
find_library(Mpg123s_LIBRARY NAMES mpg123 libmpg123)

find_package_handle_standard_args(Mpg123s
	FOUND_VAR  Mpg123s_FOUND
	REQUIRED_VARS  Mpg123s_LIBRARY  Mpg123s_INCLUDE_DIR
)

if (Mpg123s_FOUND AND NOT TARGET Mpg123s::mpg123s)
	add_library(Mpg123s::mpg123s UNKNOWN IMPORTED)
	set_target_properties(Mpg123s::mpg123s PROPERTIES
		IMPORTED_LOCATION ${Mpg123s_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${Mpg123s_INCLUDE_DIR}
	)
endif()

mark_as_advanced(Mpg123s_LIBRARY Mpg123s_INCLUDE_DIR)