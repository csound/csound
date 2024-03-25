if (NOT WIN32) # TODO: fix on windows
    generate_python_test("test_csb64enc" "Utilities" "csb64enc")
endif()
