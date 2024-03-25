option(ADD_OSC_TESTS "Add OSC tests" OFF)

if (ADD_OSC_TESTS)
    add_integration_test("OSCmidircv" "consistent" "Opcodes" "OSC")
    add_integration_test("OSCmidisend" "consistent" "Opcodes" "OSC")
endif()
