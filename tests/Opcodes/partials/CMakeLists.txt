if (NOT WIN32) # TODO: fix on other platforms
    generate_python_test("part2txt" "Opcodes" "partials")
endif()

add_integration_test("adsynt2" "consistent" "Opcodes" "partials")
add_integration_test("partials" "consistent" "Opcodes" "partials")
