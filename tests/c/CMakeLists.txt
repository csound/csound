cmake_minimum_required(VERSION 2.8)


set(TEST_ARGS "-+env:OPCODE6DIR64=${CMAKE_CURRENT_BINARY_DIR}/../..")

# Tests that depend on cunit
find_library(CUNIT_LIBRARY cunit)
check_deps(BUILD_TESTS CUNIT_LIBRARY)
if(BUILD_TESTS)

ENABLE_TESTING()

add_executable(testCompilation compilation_test.c)
target_link_libraries(testCompilation ${CSOUNDLIB} ${CUNIT_LIBRARY})
add_test(NAME testCompilation
        COMMAND $<TARGET_FILE:testCompilation> minimal.csd ${TEST_ARGS})

add_executable(testCsoundTypeSystem csound_type_system_test.c)
target_link_libraries(testCsoundTypeSystem ${CSOUNDLIB} ${CUNIT_LIBRARY})
set(testCsoundTypeSystem -D__BUILDING_LIBCSOUND)
add_test(NAME testCsoundTypeSystem
        COMMAND $<TARGET_FILE:testCsoundTypeSystem> ${TEST_ARGS})

add_executable(testCsoundMessageBuffer csound_message_buffer_test.c)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/../../H")
target_link_libraries(testCsoundMessageBuffer ${CSOUNDLIB} ${CUNIT_LIBRARY})
add_test(NAME testCsoundMessageBuffer
        COMMAND $<TARGET_FILE:testCsoundMessageBuffer> ${TEST_ARGS})

add_executable(testCsoundOrcSemantics csound_orc_semantics_test.c)
target_link_libraries(testCsoundOrcSemantics ${CSOUNDLIB} ${CUNIT_LIBRARY})
add_test(NAME testCsoundOrcSemantics
        COMMAND $<TARGET_FILE:testCsoundOrcSemantics> ${TEST_ARGS})

add_executable(testCsoundOrcCompileTest csound_orc_compile_test.c)
target_link_libraries(testCsoundOrcCompileTest ${CSOUNDLIB} ${CUNIT_LIBRARY})
add_test(NAME testCsoundOrcCompileTest
        COMMAND $<TARGET_FILE:testCsoundOrcCompileTest> ${TEST_ARGS})

add_executable(testChannels channel_tests.c)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/../../H")
target_link_libraries(testChannels ${CSOUNDLIB} ${CUNIT_LIBRARY})
add_test(NAME testChannels
        COMMAND $<TARGET_FILE:testChannels> ${TEST_ARGS})

add_executable(testCsoundDataStructures csound_data_structures_test.c)
target_link_libraries(testCsoundDataStructures ${CSOUNDLIB} ${CUNIT_LIBRARY})
add_test(NAME testCsoundDataStructures
        COMMAND $<TARGET_FILE:testCsoundDataStructures> ${TEST_ARGS})
endif()

add_executable(testIo io_test.c)
target_link_libraries(testIo ${CSOUNDLIB} ${CUNIT_LIBRARY})
add_test(NAME testIo
        COMMAND $<TARGET_FILE:testIo> minimal.csd ${TEST_ARGS})

add_executable(testCircularBuffer csound_circular_buffer_test.c)
target_link_libraries(testCircularBuffer ${CSOUNDLIB} ${CUNIT_LIBRARY})
add_test(NAME testCircularBuffer
        COMMAND $<TARGET_FILE:testCircularBuffer> minimal.csd ${TEST_ARGS})



