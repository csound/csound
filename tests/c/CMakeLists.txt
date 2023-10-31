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

    target_include_directories(unittests PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/../../H
        ${CMAKE_SOURCE_DIR}/interfaces)

    # Required because we're compiling the tests written in C as C++
    if (APPLE)
        set(CMAKE_CXX_FLAGS "-stdlib=libc++")
    else()
        set(CMAKE_CXX_FLAGS "-fpermissive -Wwrite-strings -std=c++0x")
    endif()

    target_link_libraries(unittests PRIVATE ${CSOUNDLIB_STATIC} GTest::gtest GTest::gtest_main)

    if(USE_VCPKG)
        set_property(TARGET unittests PROPERTY CXX_STANDARD 14)
    endif()

    include(GoogleTest)
    gtest_discover_tests(unittests)
    message(STATUS "Building unit tests")
else()
    message(STATUS "Not building unit tests")
endif()
