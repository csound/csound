if (NOT WIN32) # TODO: fix on other platforms
    add_integration_test("mp3scal" "consistent" "Opcodes" "mp3in")
endif()

add_integration_test("mp3in" "consistent" "Opcodes" "mp3in")
add_integration_test("mp3len" "consistent" "Opcodes" "mp3in")
