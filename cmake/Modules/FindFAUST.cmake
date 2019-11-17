# Find Faust2

include(FindPackageHandleStandardArgs)

find_path(
  FAUST_INCLUDE_DIR faust/dsp/llvm-dsp.h
  HINTS
  /opt/lib/faust/architecture/
  /usr/lib/faust/architecture/
  /usr/local/lib/faust/architecture/
  "${FAUST_INCLUDE_DIR_HINT}"
  )

set(FAUST_NAMES ${FAUST_NAMES} libfaust.so libfaust.dylib faust.dll faust libfaust)
find_library(FAUST_LIBRARY
  NAMES ${FAUST_NAMES}
  HINTS "${FAUST_LIB_DIR_HINT}")


find_package_handle_standard_args(FAUST FAUST_INCLUDE_DIR FAUST_LIBRARY)

if(FAUST_FOUND)
    set(FAUST_LIBRARIES ${FAUST_LIBRARY})
    set(FAUST_INCLUDE_DIRS ${FAUST_INCLUDE_DIR})

    if("${FAUST_LIBRARY}" MATCHES ".*\\.a")
        # This is a static build of faust, hence
        # we have to add all the LLVM flags...

        find_program(LLVM_CONFIG llvm-config HINTS /usr/bin /usr/local/bin /usr/local/opt/llvm/bin)
        if(NOT LLVM_CONFIG)
            message(WARNING "Using a static Faust library requires LLVM tooling to be present in the path.")
            UNSET(FAUST_FOUND)
        else()
        
        exec_program(${LLVM_CONFIG} ARGS --includedir OUTPUT_VARIABLE LLVM_DIR)
        exec_program(${LLVM_CONFIG} ARGS --libs OUTPUT_VARIABLE LLVM_LIBS)
        exec_program(${LLVM_CONFIG} ARGS --version OUTPUT_VARIABLE LLVM_VERSION)
        exec_program(${LLVM_CONFIG} ARGS --ldflags OUTPUT_VARIABLE LLVM_LDFLAGS)

        set(LLVM_VERSION LLVM_${LLVM_VERSION_MAJOR}${LLVM_VERSION_MINOR})

        find_package(OpenSSL REQUIRED)
        set(FAUST_LIBRARIES ${FAUST_LIBRARIES} dl ${OPENSSL_LIBRARIES} ncurses z ${LLVM_LDFLAGS} ${LLVM_LIBS} )
        endif()
        
      endif()
else()
    set(FAUST_LIBRARIES)
    set(FAUST_INCLUDE_DIRS)
endif()
