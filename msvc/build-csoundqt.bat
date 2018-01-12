cd staging
git clone -b master --depth=1 --single-branch "https://github.com/CsoundQt/CsoundQt.git"
cd CsoundQt
dir
qmake.exe qcs.pro ^
    CONFIG+=html_webengine ^
    CONFIG+=thread ^
    CONFIG+=perfThread_build ^
    CONFIG+=csound6 ^
    CONFIG+=build64 ^
    CONFIG+=QCS_QT59 ^
    QT+=xml ^
    DEFINES+="USE_DOUBLE=1" ^
    DEFINES+="USE_QT_GT_58=1" ^
    CSOUND_API_INCLUDE_DIR="%APPVEYOR_BUILD_FOLDER%\\include" ^
    CSOUND_INTERFACES_INCLUDE_DIR="%APPVEYOR_BUILD_FOLDER%\\interfaces" ^
    CSOUND_LIBRARY_DIR="%APPVEYOR_BUILD_FOLDER%\\msvc\\csound-vs\\RelWithDebInfo" ^
    DEFAULT_CSOUND_LIBS=csound64.lib ^
    CSOUND_LIBRARY=csound64.lib ^
    LCSOUND=%APPVEYOR_BUILD_FOLDER%\\msvc\\csound-vs\\RelWithDebInfo\\csound64.lib ^
    LIBS+="%VCPKGDir%\\installed\\x64-windows-static\\lib\\libsndfile.lib" ^
    LIBS+="%VCPKGDir%\\installed\\x64-windows-static\\lib\\flac.lib" ^
    LIBS+="%VCPKGDir%\\installed\\x64-windows-static\\lib\\ogg.lib" ^
    LIBS+="%VCPKGDir%\\installed\\x64-windows-static\\lib\\vorbis.lib" ^
    LIBS+="%VCPKGDir%\\installed\\x64-windows-static\\lib\\vorbisenc.lib" ^
    LIBS+="%VCPKGDir%\\installed\\x64-windows-static\\lib\\vorbisfile.lib" ^
    LIBS+="%APPVEYOR_BUILD_FOLDER%\\msvc\\csound-vs\\RelWithDebInfo\\csound64.lib" ^
    LIBS+="Ole32.lib" ^
    -after ^
    CSOUND_INTERFACES_INCLUDE_DIR+="%~dp0deps\\include" ^
    CSOUND_INTERFACES_INCLUDE_DIR+="%APPVEYOR_BUILD_FOLDER%\\deps\\include" ^
    CSOUND_INTERFACES_INCLUDE_DIR+="%VCPKGDir%\\installed\\x64-windows-static\\include" ^
    INCLUDEPATH+="%VCPKGDir%\\installed\\x64-windows-static\\include"
nmake.exe /A
move "bin\\CsoundQt*.*" "..\\..\\csound-vs\\RelWithDebInfo\\"
cd ..\..
