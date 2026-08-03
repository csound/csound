# LibLO_FOUND: if found
# LibLO::liblo: imported module

include(FindPackageHandleStandardArgs)

if(MSVC AND VCPKG_TARGET_TRIPLET MATCHES "-windows-static$")
	set(LibLO_NAMES liblo_static lo liblo)
else()
	set(LibLO_NAMES lo liblo)
endif()

find_library(LibLO_LIBRARY NAMES ${LibLO_NAMES})
find_path(LibLO_INCLUDE_DIR NAMES lo/lo.h)


find_package_handle_standard_args(LibLO
	FOUND_VAR LibLO_FOUND
	REQUIRED_VARS LibLO_LIBRARY LibLO_INCLUDE_DIR
)

if (LibLO_FOUND AND NOT TARGET LibLO::liblo)
	add_library(LibLO::liblo UNKNOWN IMPORTED)
	set_target_properties(LibLO::liblo PROPERTIES
		IMPORTED_LOCATION ${LibLO_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${LibLO_INCLUDE_DIR}
	)
endif()

mark_as_advanced(LibLO_LIBRARY LibLO_INCLUDE_DIR)
