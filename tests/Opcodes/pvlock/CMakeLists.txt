if (LINUX) # TODO: fix on other platforms
    add_integration_test("filescal" "consistent" "Opcodes" "pvlock")
endif()

add_integration_test("fmanal" "consistent" "Opcodes" "pvlock")
add_integration_test("hilbert2" "consistent" "Opcodes" "pvlock")
add_integration_test("mincer" "consistent" "Opcodes" "pvlock")
add_integration_test("pvslock" "consistent" "Opcodes" "pvlock")
add_integration_test("temposcal" "consistent" "Opcodes" "pvlock")
