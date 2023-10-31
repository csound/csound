# Opus_FOUND: whether Opus was found
# Opus::opus: imported module
# no include dir needed because it is private dependency of SndFile

include(FindPackageHandleStandardArgs)
include(CMakeFindDependencyMacro)

find_dependency(Ogg)

find_library(Opus_LIBRARY NAMES "opus")

find_package_handle_standard_args(Opus
	FOUND_VAR Opus_FOUND
	REQUIRED_VARS Opus_LIBRARY
)

if(Opus_FOUND AND NOT TARGET Opus::opus)
	add_library(Opus::opus UNKNOWN IMPORTED)
	set_target_properties(Opus::opus PROPERTIES
		IMPORTED_LOCATION ${Opus_LIBRARY}
		IMPORTED_LINK_DEPENDENT_LIBRARIES m
	)
endif()

mark_as_advanced(Opus_LIBRARY)