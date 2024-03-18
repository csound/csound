if (LINUX) # TODO: fix on other platforms
    generate_python_test("ftsave" "Opcodes" "ftgen")
endif()

add_integration_test("ftfree" "consistent" "Opcodes" "ftgen")
add_integration_test("ftgen-2" "consistent" "Opcodes" "ftgen")
add_integration_test("ftgen" "consistent" "Opcodes" "ftgen")
add_integration_test("ftgenonce" "consistent" "Opcodes" "ftgen")
add_integration_test("ftgentmp" "consistent" "Opcodes" "ftgen")
add_integration_test("getftargs" "consistent" "Opcodes" "ftgen")
