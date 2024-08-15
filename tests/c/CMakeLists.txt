
if(BUILD_TESTS)
cmake_minimum_required(VERSION 3.20)
 if(USE_VCPKG AND NOT ANDROID AND NOT EMSCRIPTEN AND NOT IOS)
    find_package(GTest CONFIG REQUIRED)
 else()
    find_package(GTest)
    check_deps(BUILD_TESTS BUILD_STATIC_LIBRARY)
 endif()
endif()

if(BUILD_TESTS)
  add_executable(unittests
        channel_tests.cpp
        csound_data_structures_test.cpp
        csound_debugger_test.cpp
        csound_message_buffer_test.cpp
        csound_orc_compile_test.cpp
        csound_orc_semantics_test.cpp
        csound_type_system_test.cpp
        csound_circular_buffer_test.cpp
        engine_test.cpp     
        io_test.cpp
        server_test.cpp
    )

   if(NOT MSVC)
        target_compile_options(unittests PRIVATE "-fpermissive" "-Wwrite-strings")
    endif()

    target_compile_features(unittests PUBLIC cxx_std_17)
    if(APPLE)
        target_compile_options(unittests PRIVATE "-fpermissive"
    "-Wno-writable-strings" "-Wno-unused-variable")
    endif()


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
