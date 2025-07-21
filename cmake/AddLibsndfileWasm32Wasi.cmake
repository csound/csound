# cmake/AddLibsndfileWasm32Wasi.cmake
function(add_libsndfile_wasm32_wasi target_name)
    include(ExternalProject)

    set(_prefix  ${CMAKE_BINARY_DIR}/_ext/${target_name})
    set(_install ${CMAKE_BINARY_DIR}/wasm32-wasi)

    ExternalProject_Add(${target_name}
        PREFIX          ${_prefix}
        GIT_REPOSITORY  https://github.com/libsndfile/libsndfile.git
        GIT_TAG         52b803f57a1f4d23471f5c5f77e1a21e0721ea0e
        UPDATE_COMMAND  ""
        PATCH_COMMAND   ${CMAKE_COMMAND} -E echo "Patching libsndfile to disable tests" &&
                        ${CMAKE_COMMAND} -E echo "set(BUILD_TESTING OFF CACHE BOOL \"\" FORCE)" > ${CMAKE_BINARY_DIR}/_ext/${target_name}/tmp/libsndfile_wasm32_wasi_ext-cache-Release.cmake
        CMAKE_ARGS
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/cmake/wasm32-wasi.cmake
            -DCMAKE_BUILD_TYPE=Release
            -DBUILD_SHARED_LIBS=OFF
            -DENABLE_EXTERNAL_LIBS=OFF     # ∅ FLAC/OGG deps
            -DENABLE_EXAMPLES=OFF
            -DBUILD_EXAMPLES=OFF
            -DENABLE_TESTS=OFF
            -DBUILD_TESTING=OFF
            -DBUILD_PROGRAMS=OFF
            -DBUILD_REGTEST=OFF
            -DENABLE_BOW_DOCS=OFF
            -DINSTALL_MANPAGES=OFF
            -DENABLE_EXPERIMENTAL=OFF
            -DCMAKE_INSTALL_PREFIX=${_install}
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DCMAKE_SYSROOT=${CMAKE_SYSROOT}
            -DWASI_RUNTIMES_LIB=${WASI_RUNTIMES_LIB}
    )

    # Don't create the imported target here - it will be created after the build
    # Just set variables that can be used later
    set(LIBSNDFILE_WASM32_LIBRARY ${_install}/lib/libsndfile.a PARENT_SCOPE)
    set(LIBSNDFILE_WASM32_INCLUDE_DIR ${_install}/include PARENT_SCOPE)
    set(LIBSNDFILE_WASM32_TARGET ${target_name} PARENT_SCOPE)
endfunction()