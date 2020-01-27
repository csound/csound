set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)

if(PORT STREQUAL "libsndfile")
	set(VCPKG_LIBRARY_LINKAGE static)
else()
	set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()
