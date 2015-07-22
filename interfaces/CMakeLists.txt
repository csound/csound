## Language bindings ##

# C++ INTERFACE LIBRARY

option(BUILD_CXX_INTERFACE "Build the C++ interface library" ON)
option(BUILD_PYTHON_INTERFACE "Build the Python interface (needs BUILD_CXX_INTERFACE)" ON)
option(BUILD_JAVA_INTERFACE "Build the Java interface (needs BUILD_CXX_INTERFACE)" ON)
option(BUILD_LUA_INTERFACE "Build the Lua interface (needs BUILD_CXX_INTERFACE)" ON)

if(BUILD_CXX_INTERFACE)
    message(STATUS "Building C++ interface library")
    set(libcsnd6_SRCS
        CppSound.cpp
        CsoundFile.cpp
        Soundfile.cpp
        csPerfThread.cpp
        cs_glue.cpp
        filebuilding.cpp)
      set(libcsnd6_LIBS ${LIBSNDFILE_LIBRARY} )
    set(libcsnd6_CFLAGS "")
    list(APPEND libcsnd6_LIBS ${CSOUNDLIB})

    add_library(libcsnd6 SHARED ${libcsnd6_SRCS})
    target_link_libraries(libcsnd6 ${libcsnd6_LIBS})
    set_target_properties(libcsnd6 PROPERTIES
        LINK_INTERFACE_LIBRARIES ""
        SOVERSION ${APIVERSION}
        OUTPUT_NAME csnd6
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        )
    if(${libcsnd6_CFLAGS})
        add_compiler_flags(${libcsnd6_CFLAGS} TARGETS libcsnd6)
    endif()

    install(TARGETS libcsnd6
        LIBRARY DESTINATION "${LIBRARY_INSTALL_DIR}"
        ARCHIVE DESTINATION "${LIBRARY_INSTALL_DIR}")

endif()


# SWIG INTERFACES
FIND_PACKAGE(SWIG 2.0)
FIND_PACKAGE(PythonLibs)
FIND_PACKAGE(JNI)
FIND_PACKAGE(Java)

find_library(LUAJIT_LIBRARY luajit)
if(NOT LUAJIT_LIBRARY)
    find_library(LUAJIT_LIBRARY luajit-5.1)
endif()
if(NOT LUAJIT_LIBRARY AND WIN32)
    find_library(LUAJIT_LIBRARY lua51)
endif()
find_path(LUA_H_PATH lua.h PATH_SUFFIXES luajit-2.0)

MESSAGE(STATUS ${PYTHON_LIBRARY})

check_deps(BUILD_PYTHON_INTERFACE SWIG_FOUND BUILD_CXX_INTERFACE PYTHONLIBS_FOUND)
check_deps(BUILD_JAVA_INTERFACE SWIG_FOUND BUILD_CXX_INTERFACE JNI_FOUND JAVA_FOUND)
check_deps(BUILD_LUA_INTERFACE SWIG_FOUND BUILD_CXX_INTERFACE  LUAJIT_LIBRARY LUA_H_PATH)

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

        if(WIN32)
            set(swigpython_LIBS ${CSOUNDLIB} ${LIBSNDFILE_LIBRARY} ${PYTHON_LIBRARY} libcsnd6)
        else()
            # Note: Do not link to PYTHON_LIBRARY so that it can be loaded in any python
            set(swigpython_LIBS ${CSOUNDLIB} ${LIBSNDFILE_LIBRARY} libcsnd6)
        endif()
# Had:     	    SWIG_FLAGS "-threads"

        SET_SOURCE_FILES_PROPERTIES(python_interface.i
            PROPERTIES CPLUSPLUS ON
            SWIG_MODULE_NAME csnd6)

        set(SWIG_MODULE_csnd6_EXTRA_DEPS ../include/csound.h ../include/cfgvar.h ../include/csound.hpp
                                        cs_glue.hpp csPerfThread.hpp CsoundFile.hpp
                                        CppSound.hpp filebuilding.h Soundfile.hpp)

        SWIG_ADD_MODULE(csnd6 python python_interface.i)
        SWIG_LINK_LIBRARIES(csnd6 ${swigpython_LIBS})

        set_target_properties(${SWIG_MODULE_csnd6_REAL_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR})

	if(APPLE)
	    set_target_properties(${SWIG_MODULE_csnd6_REAL_NAME} PROPERTIES
                LINK_FLAGS "-undefined dynamic_lookup")
	endif()

        install(TARGETS ${SWIG_MODULE_csnd6_REAL_NAME}
            LIBRARY DESTINATION "${PYTHON_MODULE_INSTALL_DIR}"
            ARCHIVE DESTINATION "${PYTHON_MODULE_INSTALL_DIR}")
        install(FILES ${BUILD_LIB_DIR}/csnd6.py
            DESTINATION ${PYTHON_MODULE_INSTALL_DIR})

    endif()

    if(BUILD_JAVA_INTERFACE)
        # The java package needs this unset or it will litter us with .java files
        unset(CMAKE_SWIG_OUTDIR)
        INCLUDE_DIRECTORIES(${JNI_INCLUDE_DIRS})

       message(STATUS "JNI INCLUDE set to ${JNI_INCLUDE_DIRS}")

      	if(WIN32)
	     set(swigjava_LIBS ${JNI_LIBRARIES} ${CSOUNDLIB}
	     ${LIBSNDFILE_LIBRARY} libcsnd6)
        elseif(APPLE)
             set(swigjava_LIBS  ${JAVA_VM_LIBRARY} ${CSOUNDLIB}
	             ${LIBSNDFILE_LIBRARY} libcsnd6)
        else()
             set(swigjava_LIBS ${CSOUNDLIB} ${LIBSNDFILE_LIBRARY} libcsnd6)
        endif()

        list(APPEND javaSwigOptions -package csnd6 -includeall)
        SET_SOURCE_FILES_PROPERTIES(java_interface.i PROPERTIES CPLUSPLUS ON)
        SET_SOURCE_FILES_PROPERTIES(java_interface.i PROPERTIES SWIG_FLAGS "${javaSwigOptions}")

        set(SWIG_MODULE__jcsound6_EXTRA_DEPS ../include/csound.h ../include/cfgvar.h ../include/csound.hpp
                                        cs_glue.hpp csPerfThread.hpp CsoundFile.hpp
                                        CppSound.hpp filebuilding.h Soundfile.hpp)

        SWIG_ADD_MODULE(_jcsound6 java java_interface.i)
        SWIG_LINK_LIBRARIES(_jcsound6 ${swigjava_LIBS})

        ADD_CUSTOM_COMMAND(TARGET _jcsound6
            POST_BUILD
            COMMAND cmake -E make_directory ./csnd6
            COMMAND ${JAVA_COMPILE} *.java -source 1.6 -target 1.6 -d .
            COMMAND ${JAVA_ARCHIVE} cf ../csnd6.jar csnd6
            COMMENT "Building JAVA package csnd6.jar")

        set_target_properties(_jcsound6 PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR})

        install(TARGETS _jcsound6
            LIBRARY DESTINATION "${JAVA_MODULE_INSTALL_DIR}"
            ARCHIVE DESTINATION "${JAVA_MODULE_INSTALL_DIR}")
        install(FILES ${BUILD_LIB_DIR}/csnd6.jar
            DESTINATION ${JAVA_MODULE_INSTALL_DIR})

    endif()

    if(BUILD_LUA_INTERFACE)
        set(swiglua_LIBS ${LUAJIT_LIBRARY} ${CSOUNDLIB} libcsnd6)

        SET_SOURCE_FILES_PROPERTIES(lua_interface.i
            PROPERTIES CPLUSPLUS ON
            SWIG_MODULE_NAME luaCsnd6)


        set(SWIG_MODULE_luaCsnd6_EXTRA_DEPS ../include/csound.h ../include/cfgvar.h ../include/csound.hpp
                                        cs_glue.hpp csPerfThread.hpp CsoundFile.hpp
                                        CppSound.hpp filebuilding.h Soundfile.hpp)

        SWIG_ADD_MODULE(luaCsnd6 lua lua_interface.i)
        SWIG_LINK_LIBRARIES(luaCsnd6 ${swiglua_LIBS})

        include_directories(${LUA_H_PATH})

      	if(APPLE)
            set_target_properties(${SWIG_MODULE_luaCsnd6_REAL_NAME} PROPERTIES
                LINK_FLAGS "-undefined dynamic_lookup")
      	endif()

        set_target_properties(${SWIG_MODULE_luaCsnd6_REAL_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            PREFIX "")

        install(TARGETS luaCsnd6
            LIBRARY DESTINATION "${LUA_MODULE_INSTALL_DIR}"
            ARCHIVE DESTINATION "${LUA_MODULE_INSTALL_DIR}")

    endif()

endif()
