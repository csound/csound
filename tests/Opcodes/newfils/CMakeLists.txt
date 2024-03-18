if (LINUX) # TODO: fix on other platforms
    add_integration_test("bob" "consistent" "Opcodes" "newfils")
endif()

if (ADD_MIDI_TESTS)
    add_integration_test("statvar" "consistent" "Opcodes" "newfils")
endif()

generate_python_test("st2ms" "Opcodes" "newfils")

add_integration_test("mvclpf4" "none" "Opcodes" "newfils")

add_integration_test("fofilter" "consistent" "Opcodes" "newfils")
add_integration_test("moogladder" "consistent" "Opcodes" "newfils")
add_integration_test("moogladder2" "consistent" "Opcodes" "newfils")
add_integration_test("ms2st" "consistent" "Opcodes" "newfils")
add_integration_test("mvchpf" "consistent" "Opcodes" "newfils")
add_integration_test("mvclpf1" "consistent" "Opcodes" "newfils")
add_integration_test("mvclpf2" "consistent" "Opcodes" "newfils")
add_integration_test("mvclpf3" "consistent" "Opcodes" "newfils")
add_integration_test("skf" "consistent" "Opcodes" "newfils")
add_integration_test("spf" "consistent" "Opcodes" "newfils")
add_integration_test("statevar" "consistent" "Opcodes" "newfils")
add_integration_test("svn" "consistent" "Opcodes" "newfils")
add_integration_test("vclpf" "consistent" "Opcodes" "newfils")
add_integration_test("vps" "consistent" "Opcodes" "newfils")
