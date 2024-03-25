if (NOT WIN32) # TODO: fix on windows
    generate_python_test("test_het_export" "Utilities" "het_export")
endif()
