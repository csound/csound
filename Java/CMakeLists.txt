cmake_policy(SET CMP0078 NEW)
cmake_policy(SET CMP0086 NEW)

## Java bindings ##
option(BUILD_JAVA_INTERFACE "Build the Java interface" ON)

# SWIG INTERFACES
find_package(SWIG 2.0)
find_package(JNI)
find_package(Java)

check_deps(BUILD_JAVA_INTERFACE SWIG_FOUND JNI_FOUND JAVA_FOUND)

if(BUILD_JAVA_INTERFACE)
    message(STATUS "SWIG_USE_FILE: " ${SWIG_USE_FILE})
    include(${SWIG_USE_FILE})
    set(CMAKE_SWIG_FLAGS ${libcsound_CFLAGS})
    list(APPEND CMAKE_SWIG_FLAGS "-includeall" "-verbose")
    if(USE_DOUBLE)
        list(APPEND CMAKE_SWIG_FLAGS "-DUSE_DOUBLE")
    endif(USE_DOUBLE)
    set(CMAKE_SWIG_OUTDIR ${BUILD_LIB_DIR})
   

    # The java package needs this unset or it will litter us with .java files
    unset(CMAKE_SWIG_OUTDIR)

         if(APPLE) 
             get_filename_component(JAVA_MODULE_INSTALL_DIR_DEFAULT "~/Library/Java/Extensions" ABSOLUTE)
        else()
            set(JAVA_MODULE_INSTALL_DIR_DEFAULT ${LIBRARY_INSTALL_DIR})
        endif()
         set(JAVA_MODULE_INSTALL_DIR ${JAVA_MODULE_INSTALL_DIR_DEFAULT} CACHE PATH "Java module install
         dir")
         message(STATUS "JAVA_MODULE_INSTALL_DIR: ${JAVA_MODULE_INSTALL_DIR}")
         message(STATUS "JNI INCLUDE set to ${JNI_INCLUDE_DIRS}.")

        list(APPEND javaSwigOptions -package csnd7 -includeall)
        SET_SOURCE_FILES_PROPERTIES(java_interface.i PROPERTIES CPLUSPLUS ON)
        SET_SOURCE_FILES_PROPERTIES(java_interface.i PROPERTIES SWIG_FLAGS "${javaSwigOptions}")

        set(SWIG_MODULE__jcsound7_EXTRA_DEPS ../include/csound.h ../include/cfgvar.h ../include/csound.hpp
                                        ../H/cs_glue.hpp ../include/csPerfThread.hpp)

        if(COMMAND SWIG_ADD_LIBRARY)
            SWIG_ADD_LIBRARY(_jcsound7 
                TYPE MODULE 
                LANGUAGE java 
                SOURCES java_interface.i cs_glue.cpp)
 	    else()
	        SWIG_ADD_MODULE(_jcsound7 java java_interface.i)
	    endif()

        target_link_libraries(_jcsound7 PUBLIC ${CSOUNDLIB} JNI::JNI)
        if (JNI_AWT_FOUND)
            target_link_libraries(_jcsound7 PRIVATE JNI::AWT)
        endif()
        if (JNI_JVM_FOUND)
            target_link_libraries(_jcsound7 PRIVATE JNI::JVM)
        endif()
        
        if(LINUX)
        set_target_properties(${SWIG_MODULE__jcsound7_REAL_NAME}
            PROPERTIES COMPILE_FLAGS "-Wno-unused-function"
            LINK_FLAGS "-Wl,-soname,lib_jcsound.so.1"
            )
         message(STATUS "Setting soname")   
         endif()
         if (APPLE)
          set_target_properties(${SWIG_MODULE__jcsound7_REAL_NAME}
            PROPERTIES COMPILE_FLAGS "-Wno-unused-function" )
         endif()

        ADD_CUSTOM_COMMAND(TARGET _jcsound7
            POST_BUILD
            COMMAND cmake -E make_directory ./csnd7
            COMMAND ${JAVA_COMPILE} *.java -source 1.8 -target 1.8 -d .
            COMMAND ${JAVA_ARCHIVE} cf ../csnd7.jar csnd7
            COMMENT "Building JAVA package csnd7.jar")

        set_target_properties(_jcsound7 PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR})
        if(${CMAKE_COMPILER_IS_GNUCC})
            message(STATUS "setting compile options for lib_jcsound")
            target_compile_options(_jcsound7 PRIVATE "-Wno-error")
         endif()

        install(TARGETS _jcsound7
            EXPORT CsoundExports
            LIBRARY DESTINATION "${JAVA_MODULE_INSTALL_DIR}"
            ARCHIVE DESTINATION "${JAVA_MODULE_INSTALL_DIR}")
        install(FILES ${BUILD_LIB_DIR}/csnd7.jar
            DESTINATION ${JAVA_MODULE_INSTALL_DIR})

endif()
