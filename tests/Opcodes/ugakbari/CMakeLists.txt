# TODO: fix stalls
# add_integration_test("scale" "consistent" "Opcodes" "ugakbari")
# add_integration_test("scale2" "consistent" "Opcodes" "ugakbari")

if (LINUX)
    add_integration_test("expcurve" "consistent" "Opcodes" "ugakbari")
endif()

add_integration_test("gainslider" "consistent" "Opcodes" "ugakbari")
add_integration_test("logcurve" "consistent" "Opcodes" "ugakbari")
