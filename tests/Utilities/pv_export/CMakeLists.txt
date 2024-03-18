if (NOT WIN32) # TODO: fix on windows
    generate_python_test("test_pv_export" "Utilities" "pv_export")
endif()