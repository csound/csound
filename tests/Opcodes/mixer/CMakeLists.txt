if (ADD_MIDI_TESTS)
    add_integration_test("vcomb" "consistent" "Opcodes" "mixer")
endif()

add_integration_test("Mixer" "consistent" "Opcodes" "mixer")