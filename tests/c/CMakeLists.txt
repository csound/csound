cmake_minimum_required(VERSION 2.8.3)

if(BUILD_TESTS)
    # Tests that depend on GTEST
    find_package(GTest REQUIRED)

    ENABLE_TESTING()

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
        io_test.cpp)

    target_link_libraries(unittests 
        GTest::GTest GTest::Main ${CSOUNDLIB_STATIC})

    target_include_directories(unittests PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/../../H
        ${CMAKE_SOURCE_DIR}/interfaces)

    add_test(AllTests unittests)

endif(BUILD_TESTS)
