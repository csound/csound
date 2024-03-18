if (ADD_MIDI_TESTS)
    add_integration_test("trigexpseg" "consistent" "Opcodes" "trigEnvSegs")
    add_integration_test("triglinseg" "consistent" "Opcodes" "trigEnvSegs")
endif()
