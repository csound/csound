option(ADD_INTEGRATION_TESTS "Add command-line tests" ON)
option(ADD_MIDI_TESTS "Add MIDI tests" OFF)
option(ADD_MIC_TESTS "Add microphone tests" OFF)

add_subdirectory(c)

find_package(Python3)

check_deps(ADD_INTEGRATION_TESTS Python3_FOUND)

# test_helpers has three functions:
#   check_input: to check an input file
#   run_command: to run a command
#   check_output: to theck an output file
# generate it in two steps, so we can use both config-time and build-time variables

# To use config-time TEST_RUNNER
configure_file("test_helpers.py.in.in" "${CMAKE_BINARY_DIR}/test_helpers.py.in")

# To use build-time TARGET_FILE_DIR:csound-bin
file(GENERATE OUTPUT "$<TARGET_FILE_DIR:csound-bin>/test_helpers.py"
    INPUT "${CMAKE_BINARY_DIR}/test_helpers.py.in"
)

# Generate a python test from a python file
# You can use generator expressions in the python file, like "$<TARGET_FILE:csbeats>"
# You can import tools from test_helpers, for example:
# `from test_helpers import check_input, run_command, check_output`
function(generate_python_test test_name)
    file(GENERATE OUTPUT "$<TARGET_FILE_DIR:csound-bin>/${test_name}.py"
        INPUT "${test_name}.py.in"
    )

    add_test(
        NAME ${test_name}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${Python3_EXECUTABLE}
            "$<TARGET_FILE_DIR:csound-bin>/${test_name}.py"
    )

    set_property(TEST ${test_name} PROPERTY LABELS ${ARGN})
endfunction()

file(GENERATE OUTPUT "$<TARGET_FILE_DIR:csound-bin>/integration.py"
    INPUT "integration.py.in"
)

# Add a csound test based on a .csd file
# in the same folder as the CMakeLists.txt file you call the function from 
# 
# Arguments:
# - filename: the filename of the .csd file
# - status: one of 
#     - "none": generates no output .wav file
#     - "indeterminate": generates an output .wav file that changes from run to run
#     - "consistent": generates a consistent output .wav file
# - labels: any number of labels for the test. You can use labels to filter which tests to run. e.g ctest -L my_label
# 
# so if filename is "array_copy", then the csd file will be "array_copy.csd"
# 
# There are three steps to running the tests:
# 1) Generate a .wav file from the csound file
#   If the status is "none", trying to generate a .wav file should yield no output. Otherwise:
# 
# 2) Calculate the checksum of the .wav file
#   Even a very small change in the .wav file will lead to a different checksum
# 
# 3a) If no checksum file exists, store the checksum
#   save the checksum in a new file in the same folder as the .csd file
#   If the filename is "array_copy", the checksum file will be "array_copy.wav.checksum"
#
# 3b) Else, compare the checksum to a stored checksum
#   If the status is "consistent", compare the new checksum to a previous saved checksum
#   If the checksums don't match, the test will fail
#   So if someone makes a change such that generated .wav files no longer sound right,
#   the tests will fail
# 
# If you want to regenerate all checksums, temporarily set REGENERATE_CHECKSUMS = True
# in the tests/integration.py.in file
function(add_integration_test test_name status)
    add_test(
        NAME ${test_name}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${Python3_EXECUTABLE} "$<TARGET_FILE_DIR:csound-bin>/integration.py"
        ${test_name} ${status}
    )

    set_property(TEST ${test_name} PROPERTY LABELS ${ARGN})
endfunction()

if (ADD_INTEGRATION_TESTS)
    add_subdirectory(commandline)
    add_subdirectory(Opcodes)
    add_subdirectory(regression)
    add_subdirectory(soak)
    add_subdirectory(Utilities)
endif()