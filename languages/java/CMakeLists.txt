cmake_policy(SET CMP0078 NEW)
cmake_policy(SET CMP0086 NEW)

if(NOT BUILD_PERFTHREAD_CLASS)
    message(FATAL_ERROR "The Java bindings require BUILD_PERFTHREAD_CLASS=ON")
endif()

find_package(SWIG 4.0 REQUIRED COMPONENTS java)
find_package(Java 11 REQUIRED COMPONENTS Development)
find_package(JNI REQUIRED)
include(UseSWIG)

set(java_source_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(java_class_dir "${CMAKE_CURRENT_BINARY_DIR}/classes")
set(java_jar "${BUILD_LIB_DIR}/csnd7.jar")
set(CMAKE_SWIG_OUTDIR "${java_source_dir}")
set(CMAKE_SWIG_FLAGS -package csnd7)
set(SWIG_USE_SWIG_DEPENDENCIES TRUE)
if(USE_DOUBLE)
    list(APPEND CMAKE_SWIG_FLAGS -DUSE_DOUBLE)
elseif(USE_FLOAT)
    list(APPEND CMAKE_SWIG_FLAGS -DUSE_FLOAT)
endif()
set_source_files_properties(java_interface.i PROPERTIES
    CPLUSPLUS ON
    USE_TARGET_INCLUDE_DIRECTORIES TRUE)
swig_add_library(_jcsound7 TYPE MODULE LANGUAGE java
    SOURCES java_interface.i cs_glue.cpp)
target_include_directories(_jcsound7 PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/H"
    "${CMAKE_BINARY_DIR}/include"
    ${JNI_INCLUDE_DIRS})
target_link_libraries(_jcsound7 PRIVATE ${CSOUNDLIB})
set_target_properties(_jcsound7 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${BUILD_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${BUILD_LIB_DIR}")
# System.loadLibrary uses .dylib on macOS, rather than CMake's MODULE .so.
if(APPLE)
    set_target_properties(_jcsound7 PROPERTIES SUFFIX ".dylib")
endif()

add_custom_command(OUTPUT "${java_jar}"
    COMMAND "${CMAKE_COMMAND}"
        "-DSOURCE_DIR=${java_source_dir}"
        "-DCLASS_DIR=${java_class_dir}"
        "-DJAR_FILE=${java_jar}"
        "-DJAVAC=${Java_JAVAC_EXECUTABLE}"
        "-DJAR=${Java_JAR_EXECUTABLE}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/BuildJava.cmake"
    DEPENDS _jcsound7 "${CMAKE_CURRENT_SOURCE_DIR}/BuildJava.cmake"
    VERBATIM)
add_custom_target(csnd7 ALL DEPENDS "${java_jar}")

set(JAVA_MODULE_INSTALL_DIR "${LIBRARY_INSTALL_DIR}" CACHE PATH
    "Install directory for csnd7.jar and its JNI library")
install(TARGETS _jcsound7
    LIBRARY DESTINATION "${JAVA_MODULE_INSTALL_DIR}"
    RUNTIME DESTINATION "${JAVA_MODULE_INSTALL_DIR}")
install(FILES "${java_jar}" DESTINATION "${JAVA_MODULE_INSTALL_DIR}")

set(java_test_dir "${CMAKE_CURRENT_BINARY_DIR}/tests")
add_custom_command(OUTPUT "${java_test_dir}/Csound7Smoke.class"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${java_test_dir}"
    COMMAND "${Java_JAVAC_EXECUTABLE}" --release 8
        -cp "${java_jar}" -d "${java_test_dir}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/Csound7Smoke.java"
    DEPENDS "${java_jar}" tests/Csound7Smoke.java
    VERBATIM)
add_custom_target(csnd7-smoke ALL
    DEPENDS "${java_test_dir}/Csound7Smoke.class")
if(WIN32)
    set(java_classpath "${java_jar}\;${java_test_dir}")
else()
    set(java_classpath "${java_jar}:${java_test_dir}")
endif()
add_test(NAME java-api
    COMMAND "${Java_JAVA_EXECUTABLE}"
        "-Djava.library.path=$<TARGET_FILE_DIR:_jcsound7>"
        -cp "${java_classpath}" Csound7Smoke)
set_tests_properties(java-api PROPERTIES TIMEOUT 30 LABELS java)
