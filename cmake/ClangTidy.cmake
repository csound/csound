find_program(PYTHON python)
find_program(PYTHON3 python3)

if(NOT PYTHON AND NOT PYTHON3)
	message(STATUS "Didn't find python, \"make format\" and \"make tidy\" will not work")
else()
	if (PYTHON3 AND NOT PYTHON)
		set(PYTHON "${PYTHON3}")
	endif()
endif()

if(PYTHON)
	find_program(CLANG_TIDY clang-tidy)

	if(NOT CLANG_TIDY)
		message(STATUS "Did not find clang-tidy, target tidy is disabled.")
	else()
		message(STATUS "Found clang-tidy, use \"make tidy\" to run it.")

		set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

		add_custom_target(tidy
			COMMAND "${PYTHON}" scripts/run-clang-tidy.py -p ${CMAKE_BINARY_DIR} -header-filter=.*
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		)
	endif()


	find_program(CLANG_FORMAT clang-format)

	if(NOT CLANG_FORMAT)
		message(STATUS "Did not find clang-format, target format is disabled.")
	else()

		message(STATUS "Found clang-format, use \"make format\" to run it.")

		add_custom_target(format
			COMMAND "${PYTHON}" scripts/run-clang-format.py -r Engine Frontends H include InOut Ops Opcodes Top util util1 util2
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		)
	endif()
endif()