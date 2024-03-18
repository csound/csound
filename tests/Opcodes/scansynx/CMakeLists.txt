if (NOT WIN32) # TODO: fix on other platforms
    add_integration_test("xscans" "consistent" "Opcodes" "scansynx")
endif()

add_integration_test("xscanmap" "consistent" "Opcodes" "scansynx")
add_integration_test("xscanu" "consistent" "Opcodes" "scansynx")
