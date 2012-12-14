cmake_minimum_required(VERSION 2.8)

ENABLE_TESTING()

set(TEST_ARGS "-+env:OPCODE6DIR64=${CMAKE_CURRENT_BINARY_DIR}/../..")

# Tests that depend on cunit
find_library(CUNIT_LIBRARY cunit)
if(CUNIT_LIBRARY)

add_executable(csoundTypeSystemTest csound_type_system_test.c)
target_link_libraries(csoundTypeSystemTest ${CSOUNDLIB} ${CUNIT_LIBRARY})
set(csoundTypeSystemTest_CFLAGS -D__BUILDING_LIBCSOUND)
add_test(NAME TestCsoundTypeSystem
	COMMAND $<TARGET_FILE:csoundTypeSystemTest> ${TEST_ARGS})

add_executable(csoundMessageBufferTest csound_message_buffer_test.c)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/../../H")
target_link_libraries(csoundMessageBufferTest ${CSOUNDLIB} cunit ${CUNIT_LIBRARY})
add_test(NAME TestCsoundMessageBuffer
	COMMAND $<TARGET_FILE:csoundMessageBufferTest> ${TEST_ARGS})
endif()



