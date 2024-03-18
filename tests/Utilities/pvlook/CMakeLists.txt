if (NOT WIN32) # TODO: fix on windows
    generate_python_test("test_pvlook" "Utilities" "pvlook")
endif()