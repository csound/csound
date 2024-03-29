## Language bindings ##

# C++ INTERFACE LIBRARY

option(BUILD_CXX_INTERFACE "Build the C++ interface library" ON)
option(BUILD_JAVA_INTERFACE "Build the Java interface (needs BUILD_CXX_INTERFACE)" ON)
option(BUILD_LUA_INTERFACE "Build the Lua interface (needs
BUILD_CXX_INTERFACE)" ON)

if(BUILD_CXX_INTERFACE)
    message(STATUS "Building C++ interface library.")
    set(libcsnd6_SRCS
        CppSound.cpp
        CsoundFile.cpp
        Soundfile.cpp
        csPerfThread.cpp
        cs_glue.cpp
        filebuilding.cpp)

    add_library(libcsnd6 SHARED ${libcsnd6_SRCS})

    if(MSVC)
        set(libcsnd6_LIBS SndFile::sndfile)
    else()
        set(libcsnd6_LIBS ${LIBSNDFILE_LIBRARY})
    endif()

    if(WIN32)
      list(APPEND libcsnd6_LIBS "${LIBSNDFILE_SUPPORT_LIBS}")
      # mingw64 needs this to create a valid .pyd.

      if(NOT MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_compile_definitions(libcsnd6 PRIVATE -DMS_WIN64)
      endif()
    endif()

    set(libcsnd6_CFLAGS "")
    list(APPEND libcsnd6_LIBS ${CSOUNDLIB})

    target_link_libraries(libcsnd6 ${libcsnd6_LIBS})
    # users of this lib will need this dir included
    target_include_directories(libcsnd6 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
    set_target_properties(libcsnd6 PROPERTIES
        SOVERSION ${APIVERSION}
        OUTPUT_NAME csnd6
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        )

   if(APPLE)
    target_compile_options(libcsnd6 PRIVATE "-std=c++11")
   endif()

    if(${libcsnd6_CFLAGS})
        target_compile_options(libcsnd6 PRIVATE ${libcsnd6_CFLAGS})
    endif()



    install(TARGETS libcsnd6
        LIBRARY DESTINATION "${LIBRARY_INSTALL_DIR}"
        ARCHIVE DESTINATION "${LIBRARY_INSTALL_DIR}")

if(BUILD_STATIC_LIBRARY)
   add_library(libcsnd6-static STATIC ${libcsnd6_SRCS})
    SET_TARGET_PROPERTIES(libcsnd6-static PROPERTIES OUTPUT_NAME csnd6)
    SET_TARGET_PROPERTIES(libcsnd6-static PROPERTIES PREFIX "lib")
    target_compile_options(libcsnd6-static PRIVATE ${libcsnd6_CFLAGS})
    
      if(APPLE)
       target_compile_options(libcsnd6-static PRIVATE "-std=c++11")
      endif()

    set_target_properties(libcsnd6-static  PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_BIN_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR})

    # Add the install target
    install(TARGETS libcsnd6-static
        LIBRARY DESTINATION "${LIBRARY_INSTALL_DIR}"
        ARCHIVE DESTINATION "${LIBRARY_INSTALL_DIR}")
endif()



else()
 message(STATUS "NOT Building C++ interface library.")
endif()


# SWIG INTERFACES
find_package(SWIG 2.0)
find_package(JNI)
find_package(Java)

# This is for Lua 5.1, it may also be possible to change 
# this to use current Lua 5.2.
find_library(LUA_LIBRARY lua5.1)
if(NOT LUA_LIBRARY AND WIN32)
    find_library(LUA_LIBRARY lua51)
endif()
find_path(LUA_H_PATH lua.h PATH_SUFFIXES lua5.1)


check_deps(BUILD_JAVA_INTERFACE SWIG_FOUND BUILD_CXX_INTERFACE JNI_FOUND JAVA_FOUND)
check_deps(BUILD_LUA_INTERFACE SWIG_FOUND BUILD_CXX_INTERFACE
LUA_LIBRARY LUA_H_PATH)


if(NOT APPLE)
      execute_process (
           COMMAND python3 -c
           "import site, sys; sys.stdout.write(site.getsitepackages()[0])"
           OUTPUT_VARIABLE PYTHON3_SITE_PACKAGES
         )
      SET(PYTHON3_MODULE_INSTALL_DIR ${PYTHON3_SITE_PACKAGES} CACHE
       PATH "Python3 module install dir")
       
      if (PYTHON3_MODULE_INSTALL_DIR)
         message(STATUS "PYTHON3 MODULE INSTALL DIR: ${PYTHON3_MODULE_INSTALL_DIR}" )
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ctcsound.py
            DESTINATION ${PYTHON3_MODULE_INSTALL_DIR})
      endif()
endif(NOT APPLE)


if(BUILD_JAVA_INTERFACE OR BUILD_LUA_INTERFACE)
    message(STATUS "SWIG_USE_FILE: " ${SWIG_USE_FILE})
    include(${SWIG_USE_FILE})
    set(CMAKE_SWIG_FLAGS ${libcsound_CFLAGS})
    list(APPEND CMAKE_SWIG_FLAGS "-includeall" "-verbose")
    if(USE_DOUBLE)
        list(APPEND CMAKE_SWIG_FLAGS "-DUSE_DOUBLE")
    endif(USE_DOUBLE)
    set(CMAKE_SWIG_OUTDIR ${BUILD_LIB_DIR})
   
    if(BUILD_JAVA_INTERFACE)
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
             set(swigjava_LIBS ${JNI_LIBRARIES} ${CSOUNDLIB}
               ${LIBSNDFILE_LIBRARY} ${LIBSNDFILE_SUPPORT_LIBS} libcsnd6)
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

        if(COMMAND SWIG_ADD_LIBRARY)
            SWIG_ADD_LIBRARY(_jcsound6 
                TYPE MODULE 
                LANGUAGE java 
                SOURCES java_interface.i)
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

    if(BUILD_LUA_INTERFACE)
        set(LUA_MODULE_INSTALL_DIR ${LIBRARY_INSTALL_DIR} CACHE PATH "Lua  module install dir")
        set(swiglua_LIBS ${LUA_LIBRARY} ${CSOUNDLIB} libcsnd6)

        SET_SOURCE_FILES_PROPERTIES(lua_interface.i
            PROPERTIES CPLUSPLUS ON
            SWIG_MODULE_NAME luaCsnd6)
            
        set(SWIG_MODULE_luaCsnd6_EXTRA_DEPS ../include/csound.h ../include/cfgvar.h ../include/csound.hpp
                                        cs_glue.hpp csPerfThread.hpp CsoundFile.hpp
                                        CppSound.hpp filebuilding.h Soundfile.hpp)

        if(COMMAND SWIG_ADD_LIBRARY)
            SWIG_ADD_LIBRARY(luaCsnd6 
                TYPE MODULE 
                LANGUAGE lua 
                SOURCES lua_interface.i)
 	    else()
	        SWIG_ADD_MODULE(luaCsnd6 lua lua_interface.i)
	    endif()
        SWIG_LINK_LIBRARIES(luaCsnd6 ${swiglua_LIBS})

        target_include_directories(${SWIG_MODULE_luaCsnd6_REAL_NAME} PRIVATE ${LUA_H_PATH})

        if(APPLE)
            set_target_properties(${SWIG_MODULE_luaCsnd6_REAL_NAME} PROPERTIES
                LINK_FLAGS "-undefined dynamic_lookup")
        endif()

        set_target_properties(${SWIG_MODULE_luaCsnd6_REAL_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            LIBRARY_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            ARCHIVE_OUTPUT_DIRECTORY ${BUILD_LIB_DIR}
            PREFIX "")
        if(${CMAKE_COMPILER_IS_GNUCC})
            target_compile_options(${SWIG_MODULE_luaCsnd6_REAL_NAME} PRIVATE "-Wno-error")
        endif()

        install(TARGETS luaCsnd6
            LIBRARY DESTINATION "${LUA_MODULE_INSTALL_DIR}"
            ARCHIVE DESTINATION "${LUA_MODULE_INSTALL_DIR}")

    endif()

endif()
