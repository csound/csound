## Language bindings ##

# C++ INTERFACE LIBRARY

option(BUILD_CXX_INTERFACE "Build the C++ interface library" ON)
option(BUILD_PYTHON_INTERFACE "Build the Python interface (needs BUILD_CXX_INTERFACE)" ON)
option(BUILD_JAVA_INTERFACE "Build the Java interface (needs BUILD_CXX_INTERFACE)" ON)
option(BUILD_LUA_INTERFACE "Build the Lua interface (needs BUILD_CXX_INTERFACE)" ON)

if(BUILD_CXX_INTERFACE)
    message(STATUS "Building C++ interface library")
    set(libcsnd_SRCS
        CppSound.cpp
        CsoundFile.cpp
        Soundfile.cpp
        csPerfThread.cpp
        cs_glue.cpp
        filebuilding.cpp)
    set(libcsnd_LIBS ${LIBSNDFILE_LIBRARY})
    set(libcsnd_CFLAGS "")
    list(APPEND libcsnd_LIBS ${CSOUNDLIB})

    add_library(libcsnd SHARED ${libcsnd_SRCS})
    target_link_libraries(libcsnd ${libcsnd_LIBS})
    set_target_properties(libcsnd PROPERTIES
        LINK_INTERFACE_LIBRARIES ""
        SOVERSION ${APIVERSION}
        OUTPUT_NAME csnd
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        )
    if(${libcsnd_CFLAGS})
         set_target_properties(libcsnd PROPERTIES
             COMPILE_FLAGS ${libcsnd_CFLAGS})
    endif()

    install(TARGETS libcsnd
        LIBRARY DESTINATION "${LIBRARY_INSTALL_DIR}"
        ARCHIVE DESTINATION "${LIBRARY_INSTALL_DIR}")
    
endif()


# SWIG INTERFACES
FIND_PACKAGE(SWIG)
FIND_PACKAGE(PythonLibs)
FIND_PACKAGE(JNI)
FIND_PACKAGE(Java)
FIND_PACKAGE(Lua51)

check_deps(BUILD_PYTHON_INTERFACE SWIG_FOUND BUILD_CXX_INTERFACE PYTHONLIBS_FOUND)
check_deps(BUILD_JAVA_INTERFACE SWIG_FOUND BUILD_CXX_INTERFACE JNI_FOUND JAVA_FOUND)
check_deps(BUILD_LUA_INTERFACE SWIG_FOUND BUILD_CXX_INTERFACE LUA51_FOUND)

if(BUILD_PYTHON_INTERFACE OR BUILD_JAVA_INTERFACE OR BUILD_LUA_INTERFACE)

    include(${SWIG_USE_FILE})

    include_directories(${CMAKE_CURRENT_SOURCE_DIR})

    set(CMAKE_SWIG_FLAGS ${libcsound_CFLAGS})
    list(APPEND CMAKE_SWIG_FLAGS "-includeall" "-verbose")

    if(USE_DOUBLE)
        list(APPEND CMAKE_SWIG_FLAGS "-DUSE_DOUBLE")
    endif(USE_DOUBLE)

    set(CMAKE_SWIG_OUTDIR ${BUILD_LIB_DIR})

    if(BUILD_PYTHON_INTERFACE)
        INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

        set(swigpython_LIBS ${PYTHON_LIBRARIES} ${CSOUNDLIB} ${LIBSNDFILE_LIBRARY} libcsnd)
    
        SET_SOURCE_FILES_PROPERTIES(python_interface.i
            PROPERTIES CPLUSPLUS ON
            SWIG_MODULE_NAME csnd)
        SWIG_ADD_MODULE(csnd python python_interface.i)
        SWIG_LINK_LIBRARIES(csnd ${swigpython_LIBS})
    
        set_target_properties(${SWIG_MODULE_csnd_REAL_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR})

        install(TARGETS ${SWIG_MODULE_csnd_REAL_NAME}
            LIBRARY DESTINATION "${PYTHON_MODULE_INSTALL_DIR}"
            ARCHIVE DESTINATION "${PYTHON_MODULE_INSTALL_DIR}")
        install(FILES ${BUILD_LIB_DIR}/csnd.py
            DESTINATION ${PYTHON_MODULE_INSTALL_DIR})

    endif()

    if(BUILD_JAVA_INTERFACE)
        # The java package needs this unset or it will litter us with .java files
        unset(CMAKE_SWIG_OUTDIR)
        INCLUDE_DIRECTORIES(${JNI_INCLUDE_DIRS})
        set(swigjava_LIBS ${JNI_LIBRARIES} ${CSOUNDLIB} ${LIBSNDFILE_LIBRARY} libcsnd)

        SET_SOURCE_FILES_PROPERTIES(java_interface.i PROPERTIES CPLUSPLUS ON)
        SET_SOURCE_FILES_PROPERTIES(java_interface.i SWIG_FLAGS "-package" "csnd")
        SWIG_ADD_MODULE(_jcsound java java_interface.i)
        SWIG_LINK_LIBRARIES(_jcsound ${swigjava_LIBS})

        ADD_CUSTOM_COMMAND(TARGET _jcsound
            POST_BUILD
            COMMAND cmake -E make_directory ./csnd
            COMMAND ${JAVA_COMPILE} *.java -d ./csnd
            COMMAND ${JAVA_ARCHIVE} cf ../csnd.jar csnd
            COMMENT "Building JAVA package csnd.jar")

        set_target_properties(_jcsound PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR})

        install(TARGETS _jcsound
            LIBRARY DESTINATION "${JAVA_MODULE_INSTALL_DIR}"
            ARCHIVE DESTINATION "${JAVA_MODULE_INSTALL_DIR}")
        install(FILES ${BUILD_LIB_DIR}/csnd.jar
            DESTINATION ${JAVA_MODULE_INSTALL_DIR})
    
    endif()

    if(BUILD_LUA_INTERFACE)
        INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIR})

        set(swiglua_LIBS ${LUA_LIBRARIES} ${CSOUNDLIB} libcsnd)
    
        SET_SOURCE_FILES_PROPERTIES(lua_interface.i
            PROPERTIES CPLUSPLUS ON
            SWIG_MODULE_NAME luaCsnd)
        SWIG_ADD_MODULE(luaCsnd lua lua_interface.i)
        SWIG_LINK_LIBRARIES(luaCsnd ${swiglua_LIBS})
    
        set_target_properties(${SWIG_MODULE_luaCsnd_REAL_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            PREFIX "")

        install(TARGETS luaCsnd
            LIBRARY DESTINATION "${LUA_MODULE_INSTALL_DIR}"
            ARCHIVE DESTINATION "${LUA_MODULE_INSTALL_DIR}")

    endif()

endif()
