if (NOT WIN32) # TODO: fix on other platforms
    add_integration_test("sequ2" "consistent" "Opcodes" "sequencer")
endif()

add_integration_test("sequ1" "consistent" "Opcodes" "sequencer")
