#ENABLE_TESTING()

set(TEST_ARGS "-+env:OPCODEDIR64=${CMAKE_CURRENT_BUILD_DIR}/../..")

add_executable(csoundTypeSystemTest csound_type_system_test.c)
target_link_libraries(csoundTypeSystemTest ${CSOUNDLIB})
set(csoundTypeSystemTest_CFLAGS -D__BUILDING_LIBCSOUND)

add_test(NAME TestCsoundTypeSystem
	COMMAND $<TARGET_FILE:csoundTypeSystemTest> ${TEST_ARGS})

