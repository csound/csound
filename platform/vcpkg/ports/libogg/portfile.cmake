vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO xiph/ogg
    REF v1.3.5
    SHA512 ecc1be8d09ebe9947128c3c8e858adb6881664b03a3a6ca9c6effae6515a0e7458ca3861ccfd5edad0c84963375dec0c7793cc15a4a25d7ec8d19734c8e60056
    HEAD_REF master
)

if(VCPKG_TARGET_IS_MINGW)
    vcpkg_replace_string("${SOURCE_PATH}/win32/ogg.def" "LIBRARY ogg" "LIBRARY libogg")
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DINSTALL_DOCS=OFF
        -DINSTALL_PKG_CONFIG_MODULE=ON
        -DBUILD_TESTING=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/Ogg PACKAGE_NAME ogg)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_copy_pdbs()
vcpkg_fixup_pkgconfig()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/COPYING")
