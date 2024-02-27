cmake_minimum_required(VERSION 3.5)

find_package(GTest)
check_deps(BUILD_TESTS BUILD_STATIC_LIBRARY GTest_FOUND)
# FIXME currently requires static lib, this should include build static csound condition
if(BUILD_TESTS)
    add_executable(unittests
        channel_tests.cpp
        cscore_tests.cpp
        csound_circular_buffer_test.cpp
        csound_data_structures_test.cpp
        csound_debugger_test.cpp
        csound_message_buffer_test.cpp
        csound_orc_compile_test.cpp
        csound_orc_semantics_test.cpp
        csound_type_system_test.cpp
        engine_test.cpp
        io_test.cpp
        server_test.cpp
    )

    target_include_directories(unittests PRIVATE ${libcsound_private_include_dirs})
    target_include_directories(unittests PUBLIC ${libcsound_public_include_dirs})

    # Required because we're compiling the tests written in C as C++
    target_compile_options(unittests PRIVATE "-fpermissive" "-Wwrite-strings")
    # gtest requires C++14
    target_compile_features(unittests PRIVATE cxx_std_14)
    target_link_libraries(unittests PRIVATE GTest::gtest_main ${CSOUNDLIB_STATIC})
 
    include(GoogleTest)
    gtest_discover_tests(unittests)
    message(STATUS "Building unit tests")
else()
    message(STATUS "Not building unit tests")
endif()
