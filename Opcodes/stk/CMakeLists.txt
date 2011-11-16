option(BUILD_STK_OPCODES "Build the stk opcodes" ON)

if(BUILD_STK_OPCODES)
    find_library(STK_LIBRARY stk)
    if(STK_LIBRARY)
        make_plugin(stkOpcodes stkOpcodes.cpp stk)
    else()
        set(stk_srcs
            src/InetWvIn.cpp    src/InetWvOut.cpp
            src/Mutex.cpp       src/RtAudio.cpp
            src/RtMidi.cpp
            src/RtWvIn.cpp      src/RtWvOut.cpp
            src/Socket.cpp      src/TcpClient.cpp
            src/TcpServer.cpp   src/Thread.cpp
            src/UdpSocket.cpp
            stkOpcodes.cpp)

        # Assume that if this directory exists, then we have the sources
        find_file(STK_FOUND
            NAMES src
            HINTS ${CMAKE_CURRENT_SOURCE_DIR})

        check_deps(BUILD_STK_OPCODES STK_FOUND)

        if(BUILD_STK_OPCODES)

            include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

            make_plugin(stk "${stk_srcs}")

            set(stkdefs "__STK_REALTIME__")
            if(APPLE)
                list(APPEND stkdefs "__OS_MACOSX__")
                list(REMOVE_ITEM "__STK_REALTIME__")
            elseif(LINUX)
                list(APPEND stkdefs "__OS_LINUX__")
                list(APPEND stkdefs "__LINUX_ALSA__")
            elseif(WIN32)
                list(APPEND stkdefs "__OS_WINDOWS__")
            endif()
    
            if(BIG_ENDIAN)
                list(APPEND stkdefs "__BIG_ENDIAN__")
            else()
                list(APPEND stkdefs "__LITTLE_ENDIAN__")
            endif()


            set_target_properties(stk PROPERTIES
                COMPILE_DEFINITIONS "${stkdefs}")

        endif()
    endif()
endif()

