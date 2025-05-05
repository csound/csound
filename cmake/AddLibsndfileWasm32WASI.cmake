# cmake/AddLibsndfileWasm32WASI.cmake
function(add_libsndfile_wasm32_wasi target_name)
    include(ExternalProject)

    set(_prefix  ${CMAKE_BINARY_DIR}/_ext/${target_name})
    set(_install ${CMAKE_BINARY_DIR}/wasm32-wasi)

    ExternalProject_Add(${target_name}
        PREFIX          ${_prefix}
        GIT_REPOSITORY  https://github.com/libsndfile/libsndfile.git
        GIT_TAG         3bd5048f8c2f7285743e9922c195c7a08f3f5551
        UPDATE_COMMAND  ""
        PATCH_COMMAND   ${CMAKE_COMMAND} -E echo "Patching libsndfile to disable tests" &&
                        ${CMAKE_COMMAND} -E echo "set(BUILD_TESTING OFF CACHE BOOL \"\" FORCE)" > ${CMAKE_BINARY_DIR}/_ext/${target_name}/tmp/libsndfile_wasm32_wasi_ext-cache-Release.cmake
        CMAKE_ARGS
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/cmake/wasm32-wasi.cmake
            -DCMAKE_BUILD_TYPE=Release
            -DBUILD_SHARED_LIBS=OFF
            -DENABLE_EXTERNAL_LIBS=OFF     # ∅ FLAC/OGG deps
            -DENABLE_EXAMPLES=OFF
            -DENABLE_TESTS=OFF
            -DBUILD_TESTING=OFF
            -DBUILD_PROGRAMS=OFF
            -DCMAKE_INSTALL_PREFIX=${_install}
    )

    add_library(libsndfile_wasm32 STATIC IMPORTED)
    set_target_properties(libsndfile_wasm32 PROPERTIES
        IMPORTED_LOCATION ${_install}/lib/libsndfile.a
        INTERFACE_INCLUDE_DIRECTORIES ${_install}/include
    )
    add_dependencies(libsndfile_wasm32 ${target_name})
endfunction()
