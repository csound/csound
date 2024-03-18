generate_python_test("dates" "Opcodes" "date")
generate_python_test("pwd" "Opcodes" "date")

add_integration_test("date" "consistent" "Opcodes" "date")
add_integration_test("readf" "consistent" "Opcodes" "date")
add_integration_test("readfi" "consistent" "Opcodes" "date")
