cd staging\CsoundQt
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
    LIBS+="%VCPKGDir%\\installed\\x64-windows\\lib\\libsndfile-1.lib" ^
    LIBS+="%APPVEYOR_BUILD_FOLDER%\\msvc\\csound-vs\\RelWithDebInfo\\csound64.lib" ^
    LIBS+="Ole32.lib" ^
    -after ^
    CSOUND_INTERFACES_INCLUDE_DIR+="%~dp0deps\\include" ^
    CSOUND_INTERFACES_INCLUDE_DIR+="%APPVEYOR_BUILD_FOLDER%\\deps\\include" ^
    CSOUND_INTERFACES_INCLUDE_DIR+="%VCPKGDir%\\installed\\x64-windows\\include" ^
    INCLUDEPATH+="%VCPKGDir%\\installed\\x64-windows\\include"
nmake.exe /A
move "bin\\CsoundQt*.*" "..\\..\\csound-vs\\RelWithDebInfo\\"
cd ..\..
