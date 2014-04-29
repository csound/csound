file(GLOB csheaders RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h")
file(GLOB csheaders_hpp RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.hpp")
list(APPEND csheaders ${csheaders_hpp})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/float-version.h.in ${CMAKE_CURRENT_BINARY_DIR}/float-version.h)

# These don't live here... but we list them here to have all headers
# in one place
list(APPEND csheaders
    ../interfaces/CsoundFile.hpp
    ../interfaces/CppSound.hpp
    ../interfaces/filebuilding.h
    ../interfaces/csPerfThread.hpp)

set(csacheaders
    ../frontends/CsoundAC/Cell.hpp
    ../frontends/CsoundAC/ChordLindenmayer.hpp
    ../frontends/CsoundAC/Composition.hpp
    ../frontends/CsoundAC/Conversions.hpp
    ../frontends/CsoundAC/Counterpoint.hpp
    ../frontends/CsoundAC/CounterpointNode.hpp
    ../frontends/CsoundAC/Event.hpp
    ../frontends/CsoundAC/Exception.hpp
    ../frontends/CsoundAC/Hocket.hpp
    ../frontends/CsoundAC/ImageToScore.hpp
    ../frontends/CsoundAC/Lindenmayer.hpp
    ../frontends/CsoundAC/MCRM.hpp
    ../frontends/CsoundAC/Midifile.hpp
    ../frontends/CsoundAC/MusicModel.hpp
    ../frontends/CsoundAC/Node.hpp
    ../frontends/CsoundAC/OrchestraNode.hpp
    ../frontends/CsoundAC/Platform.hpp
    ../frontends/CsoundAC/Random.hpp
    ../frontends/CsoundAC/Rescale.hpp
    ../frontends/CsoundAC/Score.hpp
    ../frontends/CsoundAC/ScoreModel.hpp
    ../frontends/CsoundAC/ScoreNode.hpp
    ../frontends/CsoundAC/Sequence.hpp
    ../frontends/CsoundAC/Shell.hpp
    ../frontends/CsoundAC/Silence.hpp
    ../frontends/CsoundAC/Soundfile.hpp
    ../frontends/CsoundAC/StrangeAttractor.hpp
    ../frontends/CsoundAC/System.hpp
    ../frontends/CsoundAC/Voicelead.hpp
    ../frontends/CsoundAC/VoiceleadingNode.hpp
    )

set(csvstheaders
    frontends/CsoundVST/CsoundVstFltk.hpp
    frontends/CsoundVST/CsoundVST.hpp
    frontends/CsoundVST/CsoundVstUi.h
    frontends/CsoundVST/Platform.hpp
    frontends/CsoundVST/ScoreGenerator.hpp
    frontends/CsoundVST/ScoreGeneratorVstFltk.hpp
    frontends/CsoundVST/ScoreGeneratorVst.hpp
    frontends/CsoundVST/ScoreGeneratorVstUi.h
    )

set(csheaders ${csheaders} PARENT_SCOPE)

if(NOT APPLE)
    INSTALL(FILES ${csheaders} DESTINATION ${HEADER_INSTALL_DIR})
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/float-version.h DESTINATION ${HEADER_INSTALL_DIR})
endif()

IF( BUILD_CSOUND_AC )
    INSTALL(FILES ${csacheaders} DESTINATION ${HEADER_INSTALL_DIR})
endif()

# Enable this when we build CsoundVST
#INSTALL(FILES ${csvstheaders} DESTINATION ${HEADER_INSTALL_DIR})
