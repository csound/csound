add_integration_test("dssilist" "consistent" "Opcodes" "dssi4cs")

# needs LADSPA_PATH
# and the ladspa plugins from http://plugin.org.uk/
if (ADD_DSSI_TESTS)
    add_integration_test("dssi4cs" "consistent" "Opcodes" "dssi4cs")
    add_integration_test("dssiactivate" "consistent" "Opcodes" "dssi4cs")
    add_integration_test("dssiaudio" "consistent" "Opcodes" "dssi4cs")
    add_integration_test("dssictls" "consistent" "Opcodes" "dssi4cs")
    add_integration_test("dssiinit" "consistent" "Opcodes" "dssi4cs")
    add_integration_test("dssiplay_mono" "consistent" "Opcodes" "dssi4cs")
    add_integration_test("dssiplay_stereo" "consistent" "Opcodes" "dssi4cs")
endif()
