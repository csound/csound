
set(MAX_SDK_ROOT
  "/Users/victor/src/MaxSDK-6.1.3")
set(MAX_INCLUDES_DIR 
  "${MAX_SDK_ROOT}/c74support/max-includes")
set(MSP_INCLUDES_DIR 
  "${MAX_SDK_ROOT}/c74support/msp-includes")


find_package(Boost)
find_library(MAX_AUDIO_FRAMEWORK MaxAudioAPI
  PATHS ${MSP_INCLUDES_DIR})
find_library(MAX_API_FRAMEWORK MaxAPI
  PATHS ${MAX_INCLUDES_DIR})
find_library(CORE_SERVICES_FRAMEWORK CoreServices)
find_library(LAUNCH_SERVICES_FRAMEWORK LaunchServices PATHS ${CORE_SERVICES_FRAMEWORK}/Frameworks)

option(BUILD_MAX_CSOUND_TILDE "Build csound~ for Max/MSP" ON)

check_deps(BUILD_MAX_CSOUND_TILDE
  Boost_FOUND MAX_AUDIO_FRAMEWORK MAX_API_FRAMEWORK
  CORE_SERVICES_FRAMEWORK LIBSNDFILE_LIBRARY LAUNCH_SERVICES_FRAMEWORK)

#message("${MAX_AUDIO_FRAMEWORK} : ${MAX_API_FRAMEWORK} : ${CORE_SERVICES_FRAMEWORK} : ${LIBSNDFILE_LIBRARY}")

if(BUILD_MAX_CSOUND_TILDE)

set(max_csound_tilde_SRCS
  src/Args.cpp    
  src/Lock.cpp    
  src/channel.cpp   
  src/message_buffer.cpp  
  src/util.cpp
  src/CsoundObject.cpp  
  src/PatchScripter.cpp 
  src/csound~.cpp   
  src/midi.cpp
  src/CsoundTable.cpp   
  src/atom_buffer.cpp   
  src/memory.cpp    
  src/sequencer.cpp)

set(max_csound_tilde_headers
  src/Args.h      
  src/Parser.h    
  src/csound~.h   
  src/max_headers.h   
  src/sequencer.h
  src/CsoundObject.h    
  src/PatchScripter.h   
  src/definitions.h   
  src/memory.h    
  src/util.h
  src/CsoundTable.h   
  src/atom_buffer.h   
  src/eksepshun.h   
  src/message_buffer.h
  src/Lock.h      
  src/channel.h   
  src/includes.h    
  src/midi.h)

set(maxcs_SRCS "${max_csound_tilde_SRCS};${max_csound_tilde_headers}")

#message("${maxcs_SRCS}")

#set_source_files_properties(${maxcs_SRCS} PROPERTIES 
#                              COMPILE_FLAGS "-x objective-c++")

add_library(csound~.mxo SHARED ${maxcs_SRCS})
target_link_libraries(csound~.mxo 
  ${MAX_AUDIO_FRAMEWORK} ${LIBSNDFILE_LIBRARY} ${CSOUNDLIB}
  ${CORE_SERVICES_FRAMEWORK} )
include_directories(${MAX_INCLUDES_DIR})
include_directories(${MSP_INCLUDES_DIR})
include_directories(${CORE_SERVICES_FRAMEWORK}/Frameworks/CarbonCore.framework/Headers)
include_directories(${LAUNCH_SERVICES_FRAMEWORK}/Headers)

set_target_properties(csound~.mxo PROPERTIES FRAMEWORK YES)


if(${CMAKE_GENERATOR} STREQUAL "Xcode") 
  set(CSMXO_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/Debug)
else()
  set(CSMXO_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
endif()

set(CSMXO_DIR ${CSMXO_OUT_DIR}/csound~.mxo)

add_custom_command(TARGET csound~.mxo
  POST_BUILD
  COMMAND rm ARGS -rf ${CSMXO_DIR}) 

add_custom_command(TARGET csound~.mxo
  POST_BUILD
  COMMAND mv ARGS -f ${CSMXO_OUT_DIR}/csound~.mxo.framework ${CSMXO_DIR}) 

add_custom_command(TARGET csound~.mxo
  POST_BUILD
  COMMAND mv ARGS -f ${CSMXO_DIR}/csound~.mxo  ${CSMXO_DIR}/csound~) 

add_custom_command(TARGET csound~.mxo
  POST_BUILD
  COMMAND sed ARGS 's/csound~.mxo/csound~/g' ${CSMXO_DIR}/Resources/Info.plist > ${CSMXO_DIR}/Resources/Info2.plist) 

add_custom_command(TARGET csound~.mxo
  POST_BUILD
  COMMAND mv ARGS -f ${CSMXO_DIR}/Resources/Info2.plist ${CSMXO_DIR}/Resources/Info.plist) 

# Ugh, adding this here as using the long one from C74's
# SDK causes Xcode 5 to crash
set_target_properties(csound~.mxo PROPERTIES LINK_FLAGS
  -Wl,-U,_atom_getfloat,-U,_atom_getlong,-U,_atom_getsym,-U,_atom_gettype,-U,_atom_setfloat,-U,_atom_setlong,-U,_atom_setobj,-U,_atom_setsym,-U,_attr_addfilter_clip,-U,_attr_args_process,-U,_attr_offset_new,-U,_bangout,-U,_class_addattr,-U,_class_addmethod,-U,_class_attr_addattr_format,-U,_class_attr_addattr_parse,-U,_class_attr_get,-U,_class_new,-U,_class_register,-U,_clock_fdelay,-U,_clock_new,-U,_clock_unset,-U,_defer_low,-U,_freeobject,-U,_gensym,-U,_gensym_tr,-U,_intout,-U,_jbox_get_nextobject,-U,_jbox_get_object,-U,_jpatcher_get_firstobject,-U,_listout,-U,_locatefile_extended,-U,_newobject_sprintf,-U,_object_alloc,-U,_object_attr_get_rect,-U,_object_attr_getsym,-U,_object_attr_set_rect,-U,_object_error,-U,_object_method_imp,-U,_object_method_typed,-U,_object_obex_lookup,-U,_object_post,-U,_object_warn,-U,_outlet_bang,-U,_outlet_int,-U,_outlet_list,-U,_outlet_new,-U,_path_getdefault,-U,_path_topathname,-U,_path_topotentialname,-U,_post,-U,_object_method)

endif()
