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

        if(WIN32)
             set(swigjava_LIBS ${JNI_LIBRARIES} ${CSOUNDLIB} ${LIBSNDFILE_LIBRARY})
        elseif(APPLE)
             set(swigjava_LIBS  ${JAVA_VM_LIBRARY} ${CSOUNDLIB})
        else()
             set(swigjava_LIBS ${CSOUNDLIB} ${LIBSNDFILE_LIBRARY})
        endif()

        list(APPEND javaSwigOptions -package csnd6 -includeall)
        SET_SOURCE_FILES_PROPERTIES(java_interface.i PROPERTIES CPLUSPLUS ON)
        SET_SOURCE_FILES_PROPERTIES(java_interface.i PROPERTIES SWIG_FLAGS "${javaSwigOptions}")

        set(SWIG_MODULE__jcsound6_EXTRA_DEPS ../include/csound.h ../include/cfgvar.h ../include/csound.hpp
                                        ../H/cs_glue.hpp ../include/csPerfThread.hpp)

        if(COMMAND SWIG_ADD_LIBRARY)
            SWIG_ADD_LIBRARY(_jcsound6 
                TYPE MODULE 
                LANGUAGE java 
                SOURCES java_interface.i cs_glue.cpp)
 	    else()
	        SWIG_ADD_MODULE(_jcsound6 java java_interface.i)
	    endif()
        SWIG_LINK_LIBRARIES(_jcsound6 ${swigjava_LIBS})
        target_include_directories(${SWIG_MODULE__jcsound6_REAL_NAME}
        PRIVATE ${JNI_INCLUDE_DIRS})
        if(LINUX)
        set_target_properties(${SWIG_MODULE__jcsound6_REAL_NAME}
            PROPERTIES COMPILE_FLAGS "-Wno-unused-function"
            LINK_FLAGS "-Wl,-soname,lib_jcsound.so.1"
            )
         message(STATUS "Setting soname")   
         endif()
         if (APPLE)
          set_target_properties(${SWIG_MODULE__jcsound6_REAL_NAME}
            PROPERTIES COMPILE_FLAGS "-Wno-unused-function" )
         endif()

        ADD_CUSTOM_COMMAND(TARGET _jcsound6
            POST_BUILD
            COMMAND cmake -E make_directory ./csnd6
            COMMAND ${JAVA_COMPILE} *.java -source 1.8 -target 1.8 -d .
            COMMAND ${JAVA_ARCHIVE} cf ../csnd6.jar csnd6
            COMMENT "Building JAVA package csnd6.jar")

        set_target_properties(_jcsound6 PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR})
        if(${CMAKE_COMPILER_IS_GNUCC})
            message(STATUS "setting compile options for lib_jcsound")
            target_compile_options(_jcsound6 PRIVATE "-Wno-error")
         endif()

        install(TARGETS _jcsound6
            LIBRARY DESTINATION "${JAVA_MODULE_INSTALL_DIR}"
            ARCHIVE DESTINATION "${JAVA_MODULE_INSTALL_DIR}")
        install(FILES ${BUILD_LIB_DIR}/csnd6.jar
            DESTINATION ${JAVA_MODULE_INSTALL_DIR})

endif()
