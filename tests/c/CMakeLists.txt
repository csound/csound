cmake_minimum_required(VERSION 2.8)


set(TEST_ARGS "-+env:OPCODE6DIR64=${CMAKE_CURRENT_BINARY_DIR}/../..")

# Tests that depend on cunit
find_library(CUNIT_LIBRARY cunit)
check_deps(BUILD_TESTS CUNIT_LIBRARY BUILD_STATIC_LIBRARY)
if(BUILD_TESTS)

ENABLE_TESTING()

add_executable(testCsoundTypeSystem csound_type_system_test.c)
target_link_libraries(testCsoundTypeSystem ${CSOUNDLIB_STATIC} ${CUNIT_LIBRARY})
set(testCsoundTypeSystem -D__BUILDING_LIBCSOUND)
add_test(NAME testCsoundTypeSystem
        COMMAND $<TARGET_FILE:testCsoundTypeSystem> ${TEST_ARGS})

add_executable(testCsoundMessageBuffer csound_message_buffer_test.c)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/../../H")
target_link_libraries(testCsoundMessageBuffer ${CSOUNDLIB_STATIC} ${CUNIT_LIBRARY})
add_test(NAME testCsoundMessageBuffer
        COMMAND $<TARGET_FILE:testCsoundMessageBuffer> ${TEST_ARGS})

add_executable(testCsoundOrcSemantics csound_orc_semantics_test.c)
target_link_libraries(testCsoundOrcSemantics ${CSOUNDLIB_STATIC} ${CUNIT_LIBRARY})
add_test(NAME testCsoundOrcSemantics
        COMMAND $<TARGET_FILE:testCsoundOrcSemantics> ${TEST_ARGS})

add_executable(testCsoundOrcCompileTest csound_orc_compile_test.c)
target_link_libraries(testCsoundOrcCompileTest ${CSOUNDLIB_STATIC} ${CUNIT_LIBRARY})
add_test(NAME testCsoundOrcCompileTest
        COMMAND $<TARGET_FILE:testCsoundOrcCompileTest> ${TEST_ARGS})

add_executable(testChannels channel_tests.c)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/../../H")
target_link_libraries(testChannels ${CSOUNDLIB_STATIC} ${CUNIT_LIBRARY})
add_test(NAME testChannels
        COMMAND $<TARGET_FILE:testChannels> ${TEST_ARGS})

add_executable(testCsoundDataStructures csound_data_structures_test.c)
target_link_libraries(testCsoundDataStructures ${CSOUNDLIB_STATIC} ${CUNIT_LIBRARY})
add_test(NAME testCsoundDataStructures
        COMMAND $<TARGET_FILE:testCsoundDataStructures> ${TEST_ARGS})

add_executable(testIo io_test.c)
target_link_libraries(testIo ${CSOUNDLIB_STATIC} ${CUNIT_LIBRARY})
add_test(NAME testIo
        COMMAND $<TARGET_FILE:testIo> ${TEST_ARGS})

add_executable(testCircularBuffer csound_circular_buffer_test.c)
target_link_libraries(testCircularBuffer ${CSOUNDLIB_STATIC} ${CUNIT_LIBRARY} pthread)
add_test(NAME testCircularBuffer
        COMMAND $<TARGET_FILE:testCircularBuffer> minimal.csd ${TEST_ARGS})

add_executable(testCscore cscore_tests.c)
target_link_libraries(testCscore ${CSOUNDLIB} ${CUNIT_LIBRARY} pthread)
add_test(NAME testCscore
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/c/
        COMMAND $<TARGET_FILE:testCscore> ${CMAKE_SOURCE_DIR}/tests/c/ -arg2 ${TEST_ARGS})

include_directories(${CMAKE_SOURCE_DIR}/interfaces)
add_executable(testPerfThread perfthread_test.cpp)
target_link_libraries(testPerfThread ${CSOUNDLIB} ${CUNIT_LIBRARY} pthread libcsnd6)
add_test(NAME testPerfThread
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/c/
        COMMAND $<TARGET_FILE:testPerfThread> ${CMAKE_SOURCE_DIR}/tests/c/ -arg2 ${TEST_ARGS})

add_executable(testDebugger csound_debugger_test.c)
target_link_libraries(testDebugger ${CSOUNDLIB} ${CUNIT_LIBRARY} pthread libcsnd6)
add_test(NAME testDebugger
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/c/
        COMMAND $<TARGET_FILE:testDebugger> ${CMAKE_SOURCE_DIR}/tests/c/ -arg2 ${TEST_ARGS})


add_executable(testEngine engine_test.c)
target_link_libraries(testEngine ${CSOUNDLIB} ${CUNIT_LIBRARY} pthread)
add_test(NAME testEngine
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/c/
        COMMAND $<TARGET_FILE:testEngine> ${CMAKE_SOURCE_DIR}/tests/c/ -arg2 ${TEST_ARGS})


endif(BUILD_TESTS)




