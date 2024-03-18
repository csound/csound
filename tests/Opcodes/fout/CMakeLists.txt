if (ADD_MIDI_TESTS)
    add_integration_test("foutir" "consistent" "Opcodes" "fout")
endif()

if (NOT WIN32) # TODO: fix on other platforms
    generate_python_test("fiopen" "Opcodes" "fout")
    generate_python_test("ficlose" "Opcodes" "fout")
    generate_python_test("fprints" "Opcodes" "fout")
    generate_python_test("fprintks" "Opcodes" "fout")
    generate_python_test("fprintks-2" "Opcodes" "fout")
endif()

generate_python_test("fout" "Opcodes" "fout")
generate_python_test("foutk" "Opcodes" "fout")
generate_python_test("fout_poly" "Opcodes" "fout")

add_integration_test("clear" "consistent" "Opcodes" "fout")
add_integration_test("fin" "consistent" "Opcodes" "fout")
add_integration_test("fini" "consistent" "Opcodes" "fout")
add_integration_test("fink" "consistent" "Opcodes" "fout")
add_integration_test("fout_ftable" "consistent" "Opcodes" "fout")
add_integration_test("fouti" "consistent" "Opcodes" "fout")
add_integration_test("vincr-complex" "consistent" "Opcodes" "fout")
add_integration_test("vincr" "consistent" "Opcodes" "fout")
