set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES x86_64)

set(VCPKG_OSX_SYSROOT $ENV{OSXCROSS_SDK})

get_filename_component(TOOLCHAIN_FILE "../platform/osxcross/osx-x64.cmake" REALPATH BASE_DIR "${CMAKE_SOURCE_DIR}")
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${TOOLCHAIN_FILE})
