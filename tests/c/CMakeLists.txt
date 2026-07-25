
if(BUILD_TESTS)
cmake_minimum_required(VERSION 3.20)
include(CTest)
 if(USE_VCPKG AND NOT ANDROID AND NOT EMSCRIPTEN AND NOT IOS)
    find_package(GTest CONFIG REQUIRED)
 else()
    find_package(GTest)
 endif()
    check_deps(BUILD_TESTS BUILD_STATIC_LIBRARY)
endif()

if(BUILD_TESTS)
  add_executable(unittests
        channel_tests.cpp
        csound_data_structures_test.cpp
        csound_debug_callback_test.cpp
        csound_debugger_test.cpp
        csound_message_buffer_test.cpp
        csound_option_parser_test.cpp
        csound_orc_compile_test.cpp
        csound_orc_semantics_test.cpp
        csound_type_system_test.cpp
        csound_circular_buffer_test.cpp
        engine_test.cpp
        io_test.cpp
        osc_blob_test.cpp
        server_test.cpp
        csound_plugin_opcode_test.cpp
        perfthread_test.cpp
        csound_test_sndfile.cpp
        test_new_type.cpp
        test_pfields.cpp
        ugen_test.cpp
        ../../Opcodes/osc_blob.c
    )

    set(unittests_CFLAGS ${libcsound_CFLAGS})
    list(REMOVE_ITEM unittests_CFLAGS -D__BUILDING_LIBCSOUND)
    target_compile_options(unittests PRIVATE ${unittests_CFLAGS})
    target_compile_features(unittests PUBLIC cxx_std_17)

    target_include_directories(${CSOUNDLIB} PRIVATE ${libcsound_private_include_dirs})
    target_include_directories(${CSOUNDLIB} PUBLIC ${libcsound_public_include_dirs})

    target_link_libraries(unittests
        PRIVATE
            GTest::gtest_main
            ${CSOUNDLIB_STATIC}
    )

    include(GoogleTest)
    gtest_discover_tests(unittests)
    message(STATUS "Building unit tests")
else()
    message(STATUS "Not building unit tests")
endif()
