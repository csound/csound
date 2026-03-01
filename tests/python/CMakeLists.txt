
find_package(Python3 COMPONENTS Interpreter)

if(Python3_FOUND)
    # Collect all test_*.py files in this directory
    file(GLOB PYTHON_TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/test_*.py")

    if(PYTHON_TEST_FILES)
        # On macOS the build-tree framework must be discoverable
        if(APPLE)
            set(_pytest_env
                "DYLD_FRAMEWORK_PATH=${CMAKE_BINARY_DIR}"
                "PYTHONPATH=${CMAKE_SOURCE_DIR}/Python")
        elseif(WIN32)
            set(_pytest_env
                "PATH=${CMAKE_BINARY_DIR};$ENV{PATH}"
                "PYTHONPATH=${CMAKE_SOURCE_DIR}/Python")
        else()
            set(_pytest_env
                "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}"
                "PYTHONPATH=${CMAKE_SOURCE_DIR}/Python")
        endif()

        add_custom_target(pytests
            COMMAND ${CMAKE_COMMAND} -E env ${_pytest_env}
                    ${Python3_EXECUTABLE} -m unittest discover
                    -s ${CMAKE_CURRENT_SOURCE_DIR}
                    -p "test_*.py" -v
            DEPENDS ${CSOUNDLIB}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running Python tests via unittest"
            USES_TERMINAL
        )
    else()
        message(STATUS "No Python test files found in ${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
else()
    message(STATUS "Python3 not found – skipping pytests target")
endif()
