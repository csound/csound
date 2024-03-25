if (NOT WIN32) # TODO: fix on other platforms
    add_integration_test("OSCsend" "consistent" "Opcodes" "socksend")
endif()

add_integration_test("oscbundle" "consistent" "Opcodes" "socksend")