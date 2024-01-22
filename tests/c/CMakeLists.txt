cmake_minimum_required(VERSION 3.5)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

if(USE_VCPKG)
    find_package(GTest CONFIG REQUIRED)
else()
    find_package(GTest)
    check_deps(BUILD_TESTS
        BUILD_STATIC_LIBRARY GTest_FOUND GTEST_BOTH_LIBRARIES GTEST_INCLUDE_DIRS
    )
endif()

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

    target_include_directories(unittests PRIVATE ${LIBCSOUND_PRIVATE_INCLUDES})
    target_include_directories(unittests PUBLIC ${LIBCSOUND_PUBLIC_INCLUDES})

    # Required because we're compiling the tests written in C as C++
    if(NOT MSVC)
        target_compile_options(unittests PRIVATE "-fpermissive" "-Wwrite-strings")
    endif()

    target_compile_features(unittests PUBLIC cxx_std_17)

    if(USE_VCPKG)
        target_link_libraries(unittests PRIVATE
            GTest::gtest_main ${CSOUNDLIB_STATIC})
    else()
        target_include_directories(unittests PRIVATE ${GTEST_INCLUDE_DIRS})
        target_link_libraries(unittests PRIVATE ${GTEST_BOTH_LIBRARIES} ${CSOUNDLIB_STATIC})
    endif()

    include(GoogleTest)
    gtest_discover_tests(unittests)

    message(STATUS "Building unit tests")
else()
    message(STATUS "Not building unit tests")
endif()
