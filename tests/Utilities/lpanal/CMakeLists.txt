if (LINUX) # TODO: fix on other platforms
    generate_python_test("lpanal1" "Utilities" "lpanal")
    generate_python_test("lpanal2" "Utilities" "lpanal")
    generate_python_test("lpanal3" "Utilities" "lpanal")
endif()