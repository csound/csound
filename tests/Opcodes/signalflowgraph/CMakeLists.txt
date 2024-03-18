if (NOT WIN32) # TODO: fix on windows
    add_integration_test("signalflowgraphtest" "consistent" "Opcodes" "signalflowgraph")
endif()

add_integration_test("alwayson" "consistent" "Opcodes" "signalflowgraph")
add_integration_test("connect" "consistent" "Opcodes" "signalflowgraph")
add_integration_test("inleta" "consistent" "Opcodes" "signalflowgraph")
add_integration_test("inletk" "consistent" "Opcodes" "signalflowgraph")
add_integration_test("outleta" "consistent" "Opcodes" "signalflowgraph")
add_integration_test("outletk" "consistent" "Opcodes" "signalflowgraph")
add_integration_test("signalflowgraph" "consistent" "Opcodes" "signalflowgraph")