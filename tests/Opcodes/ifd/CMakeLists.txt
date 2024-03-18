if (NOT WIN32) # TODO: fix on other platforms
    add_integration_test("tabifd" "consistent" "Opcodes" "ifd")
endif()

add_integration_test("pvsifd" "consistent" "Opcodes" "ifd")