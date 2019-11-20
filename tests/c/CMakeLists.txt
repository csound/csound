cmake_minimum_required(VERSION 3.5)

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

    if(MSVC)
        find_package(GTest CONFIG REQUIRED)

        target_link_libraries(unittests PRIVATE
            GTest::gtest_main ${CSOUNDLIB_STATIC})
    else ()
        find_package(GTest REQUIRED)

        target_include_directories(unittests PRIVATE
            ${GTEST_INCLUDE_DIRS})

        target_link_libraries(unittests PRIVATE
            GTest::GTest GTest::Main ${CSOUNDLIB_STATIC})
    endif()

    add_test(NAME AllTests COMMAND unittests)

endif()
