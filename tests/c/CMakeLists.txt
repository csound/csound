cmake_minimum_required(VERSION 2.8)


set(TEST_ARGS "-+env:OPCODE6DIR64=${CMAKE_CURRENT_BINARY_DIR}/../..")

# Tests that depend on cunit
find_library(CUNIT_LIBRARY cunit)
check_deps(BUILD_TESTS CUNIT_LIBRARY)
if(BUILD_TESTS)

ENABLE_TESTING()

add_executable(compilationTest compilation_test.c)
target_link_libraries(compilationTest ${CSOUNDLIB} ${CUNIT_LIBRARY})
add_test(NAME compilationTest
	COMMAND $<TARGET_FILE:compilationTest> minimal.csd ${TEST_ARGS})

add_executable(csoundTypeSystemTest csound_type_system_test.c)
target_link_libraries(csoundTypeSystemTest ${CSOUNDLIB} ${CUNIT_LIBRARY})
set(csoundTypeSystemTest_CFLAGS -D__BUILDING_LIBCSOUND)
add_test(NAME TestCsoundTypeSystem
	COMMAND $<TARGET_FILE:csoundTypeSystemTest> ${TEST_ARGS})

add_executable(csoundMessageBufferTest csound_message_buffer_test.c)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/../../H")
target_link_libraries(csoundMessageBufferTest ${CSOUNDLIB} ${CUNIT_LIBRARY})
add_test(NAME TestCsoundMessageBuffer
	COMMAND $<TARGET_FILE:csoundMessageBufferTest> ${TEST_ARGS})

add_executable(csoundOrcSemanticsTest csound_orc_semantics_test.c)
target_link_libraries(csoundOrcSemanticsTest ${CSOUNDLIB} ${CUNIT_LIBRARY})
add_test(NAME csoundOrcSemanticsTest
	COMMAND $<TARGET_FILE:csoundOrcSemanticsTest> ${TEST_ARGS})

add_executable(csoundOrcCompileTest csound_orc_compile_test.c)
target_link_libraries(csoundOrcCompileTest ${CSOUNDLIB} ${CUNIT_LIBRARY})
add_test(NAME csoundOrcCompileTest
	COMMAND $<TARGET_FILE:csoundOrcCompileTest> ${TEST_ARGS})

add_executable(channelTest channel_tests.c)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/../../H")
target_link_libraries(channelTest ${CSOUNDLIB} ${CUNIT_LIBRARY})
add_test(NAME channelTest
	COMMAND $<TARGET_FILE:channelTest> ${TEST_ARGS})

add_executable(csoundDataStructuresTest csound_data_structures_test.c)
target_link_libraries(csoundDataStructuresTest ${CSOUNDLIB} ${CUNIT_LIBRARY})
add_test(NAME csoundDataStructuresTest
	COMMAND $<TARGET_FILE:csoundDataStructuresTest> ${TEST_ARGS})
endif()

add_executable(ioTest io_test.c)
target_link_libraries(ioTest ${CSOUNDLIB} ${CUNIT_LIBRARY})
add_test(NAME ioTest
        COMMAND $<TARGET_FILE:ioTest> minimal.csd ${TEST_ARGS})



