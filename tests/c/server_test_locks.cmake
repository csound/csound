# Server tests release their UDP port probes before starting listeners.
# Keep another server test from claiming a port during that gap; other
# unit tests can still run in parallel. Run this after GoogleTest discovery.
foreach(test_name IN LISTS unittests_TESTS)
    if(test_name MATCHES "^ServerTests\\.")
        set_tests_properties("${test_name}" PROPERTIES
            RESOURCE_LOCK csound_udp_ports)
    endif()
endforeach()
unset(test_name)
