option(ADD_INTEGRATION_TESTS "Add command-line tests" ON)

add_subdirectory(c)

find_package(Python3)

check_deps(ADD_INTEGRATION_TESTS Python3_FOUND)

# To use config-time TEST_RUNNER
configure_file(
    "test_helpers.py.in.in"
    "${CMAKE_BINARY_DIR}/test_helpers.py.in"
)

# To use build-time TARGET_FILE_DIR
file(GENERATE OUTPUT "$<TARGET_FILE_DIR:csound-bin>/test_helpers.py"
    INPUT "${CMAKE_BINARY_DIR}/test_helpers.py.in"
)

file(GENERATE OUTPUT "$<TARGET_FILE_DIR:csound-bin>/integration.py"
    INPUT "integration.py.in"
)

if (ADD_INTEGRATION_TESTS)
    add_subdirectory(commandline)
    add_subdirectory(regression)
    add_subdirectory(soak)
endif()