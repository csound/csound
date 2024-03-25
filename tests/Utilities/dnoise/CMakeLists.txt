if (LINUX) # TODO: fix on other platforms
    generate_python_test("dnoise1" "Utilities" "dnoise")
    generate_python_test("dnoise2" "Utilities" "dnoise")
    generate_python_test("dnoise3" "Utilities" "dnoise")
endif()
