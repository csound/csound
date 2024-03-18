option(ADD_ARDUINO_TESTS "Add arduino tests" OFF)

if (ADD_ARDUINO_TESTS)
    add_integration_test("arduinoRead" "none" "Opcodes" "serial")
    add_integration_test("arduinoReadF" "none" "Opcodes" "serial")
endif()

if (LINUX) # TODO: fix on other platforms
    add_integration_test("serialBegin" "consistent" "Opcodes" "serial")
    add_integration_test("serialRead" "consistent" "Opcodes" "serial")
    add_integration_test("serialWrite" "consistent" "Opcodes" "serial")
endif()
