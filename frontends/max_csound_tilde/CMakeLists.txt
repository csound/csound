if(MAX_SDK_ROOT)
  message(STATUS "Building csound~ for Max/MSP")
  message(STATUS "  MAX_SDK_ROOT: ${MAX_SDK_ROOT}")

set(MAX_INCLUDES_DIR
  "${MAX_SDK_ROOT}/c74support/max-includes")
set(MSP_INCLUDES_DIR
  "${MAX_SDK_ROOT}/c74support/msp-includes")


find_package(Boost)

if(APPLE)
  find_library(CORE_SERVICES_FRAMEWORK CoreServices)
  find_library(LAUNCH_SERVICES_FRAMEWORK LaunchServices PATHS ${CORE_SERVICES_FRAMEWORK}/Frameworks)
  find_library(MAX_AUDIO_FRAMEWORK MaxAudioAPI
    PATHS ${MSP_INCLUDES_DIR})
  find_library(MAX_API_FRAMEWORK MaxAPI
    PATHS ${MAX_INCLUDES_DIR})
else()

  find_library(MAX_AUDIO_FRAMEWORK MaxAudio
    PATHS ${MSP_INCLUDES_DIR})
  find_library(MAX_API_FRAMEWORK MaxAPI
    PATHS ${MAX_INCLUDES_DIR})
endif()

option(BUILD_MAX_CSOUND_TILDE "Build csound~ for Max/MSP" ON)


if(APPLE)
  check_deps(BUILD_MAX_CSOUND_TILDE
    Boost_FOUND MAX_AUDIO_FRAMEWORK MAX_API_FRAMEWORK
    #CORE_SERVICES_FRAMEWORK LIBSNDFILE_LIBRARY LAUNCH_SERVICES_FRAMEWORK)
    CORE_SERVICES_FRAMEWORK LAUNCH_SERVICES_FRAMEWORK)
else()
  check_deps(BUILD_MAX_CSOUND_TILDE
    MAX_AUDIO_FRAMEWORK MAX_API_FRAMEWORK
    Boost_INCLUDE_DIRS LIBSNDFILE_LIBRARY)
endif()

#message("${MAX_AUDIO_FRAMEWORK} : ${MAX_API_FRAMEWORK} : ${CORE_SERVICES_FRAMEWORK} : ${LIBSNDFILE_LIBRARY}")

if(BUILD_MAX_CSOUND_TILDE)

set(max_csound_tilde_SRCS
  src/Args.cpp
  src/atom_buffer.cpp
  src/channel.cpp
  src/csound~.cpp
  src/CsoundObject.cpp
  src/CsoundTable.cpp
  src/Lock.cpp
  src/memory.cpp
  src/message_buffer.cpp
  src/midi.cpp
  src/PatchScripter.cpp
  src/sequencer.cpp
  src/util.cpp)

if(NOT APPLE)
  list(APPEND max_csound_tilde_SRCS
    src/dllmain_win.c)
endif()

set(max_csound_tilde_headers
  src/Args.h
  src/atom_buffer.h
  src/channel.h
  src/csound~.h
  src/CsoundObject.h
  src/CsoundTable.h
  src/definitions.h
  src/eksepshun.h
  src/includes.h
  src/Lock.h
  src/max_headers.h
  src/memory.h
  src/message_buffer.h
  src/midi.h
  src/Parser.h
  src/PatchScripter.h
  src/sequencer.h
  src/util.h)

set(maxcs_SRCS "${max_csound_tilde_SRCS};${max_csound_tilde_headers}")

#message("${maxcs_SRCS}")

#set_source_files_properties(${maxcs_SRCS} PROPERTIES
#                              COMPILE_FLAGS "-x objective-c++")

if(APPLE)
  set(MAX_OUTPUT_LIB "csound~.mxo")
else()
  set(MAX_OUTPUT_LIB "csound~")
endif()

add_library(${MAX_OUTPUT_LIB} SHARED ${maxcs_SRCS})
target_link_libraries(${MAX_OUTPUT_LIB}
  ${MAX_API_FRAMEWORK} ${MAX_AUDIO_FRAMEWORK}
  #${LIBSNDFILE_LIBRARY} ${CSOUNDLIB})
  ${CSOUNDLIB})
include_directories(${MAX_INCLUDES_DIR})
include_directories(${MSP_INCLUDES_DIR})
include_directories(${Boost_INCLUDE_DIRS})


if(NOT APPLE)
  set_target_properties(${MAX_OUTPUT_LIB} 
	  PROPERTIES LINK_FLAGS "-static-libstdc++ -static-libgcc")

endif()

if(APPLE)

include_directories(${CORE_SERVICES_FRAMEWORK}/Frameworks/CarbonCore.framework/Headers)
include_directories(${LAUNCH_SERVICES_FRAMEWORK}/Headers)
target_link_libraries(${MAX_OUTPUT_LIB} ${CORE_SERVICES_FRAMEWORK})
set_target_properties(${MAX_OUTPUT_LIB} PROPERTIES FRAMEWORK YES)


if(${CMAKE_GENERATOR} STREQUAL "Xcode")
  set(CSMXO_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
else()
  set(CSMXO_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
endif()

set(CSMXO_DIR ${CSMXO_OUT_DIR}/csound~.mxo)

add_custom_command(TARGET ${MAX_OUTPUT_LIB}
  POST_BUILD
  COMMAND rm ARGS -rf ${CSMXO_DIR})

add_custom_command(TARGET ${MAX_OUTPUT_LIB}
  POST_BUILD
  COMMAND cp ARGS -r ${CSMXO_OUT_DIR}/csound~.mxo.framework ${CSMXO_DIR})

add_custom_command(TARGET ${MAX_OUTPUT_LIB}
  POST_BUILD
  COMMAND mv ARGS -f ${CSMXO_DIR}/csound~.mxo  ${CSMXO_DIR}/csound~)

add_custom_command(TARGET ${MAX_OUTPUT_LIB}
  POST_BUILD
  COMMAND sed ARGS 's/csound~.mxo/csound~/g' ${CSMXO_DIR}/Resources/Info.plist > ${CSMXO_DIR}/Resources/Info2.plist)

add_custom_command(TARGET ${MAX_OUTPUT_LIB}
  POST_BUILD
  COMMAND mv ARGS -f ${CSMXO_DIR}/Resources/Info2.plist ${CSMXO_DIR}/Resources/Info.plist)

set(MAX_LINK_FLAGS "-Wl,-U,_atom_getfloat,-U,_atom_getlong,-U,_atom_getsym,-U,_atom_gettype,-U,_atom_setfloat,-U,_atom_setlong,-U,_atom_setobj,-U,_atom_setsym,-U,_attr_addfilter_clip,-U,_attr_args_process,-U,_attr_offset_new,-U,_bangout,-U,_class_addattr,-U,_class_addmethod,-U,_class_attr_addattr_format,-U,_class_attr_addattr_parse,-U,_class_attr_get,-U,_class_new,-U,_class_register,-U,_clock_fdelay,-U,_clock_new,-U,_clock_unset,-U,_defer_low,-U,_freeobject,-U,_gensym,-U,_gensym_tr,-U,_intout,-U,_jbox_get_nextobject,-U,_jbox_get_object,-U,_jpatcher_get_firstobject,-U,_listout,-U,_locatefile_extended,-U,_newobject_sprintf,-U,_object_alloc,-U,_object_attr_get_rect,-U,_object_attr_getsym,-U,_object_attr_set_rect,-U,_object_error,-U,_object_method_imp,-U,_object_method_typed,-U,_object_obex_lookup,-U,_object_post,-U,_object_warn,-U,_outlet_bang,-U,_outlet_int,-U,_outlet_list,-U,_outlet_new,-U,_path_getdefault,-U,_path_topathname,-U,_path_topotentialname,-U,_post,-U,_object_method")

# Ugh, adding this here as using the long one from C74's
# SDK causes Xcode 5 to crash
# See if this will allow csound~ to pickup csound's libsndfile symbols at runtime...
set_target_properties(csound~.mxo PROPERTIES LINK_FLAGS
  "${MAX_LINK_FLAGS} -undefined dynamic_lookup")

else()

set_target_properties(${MAX_OUTPUT_LIB} PROPERTIES
  COMPILE_FLAGS "-DWIN_VERSION -DWIN_EXT_VERSION -D_WINDOWS"
  SUFFIX .mxe
  )

target_link_libraries(${MAX_OUTPUT_LIB}
  ${LIBSNDFILE_LIBRARY})

endif(APPLE)

endif()

else()
  message(STATUS "Not Building csound~ for Max/MSP")
  message(STATUS "  MAX_SDK_ROOT is not defined")

endif()
