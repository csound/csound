echo TODO: This is a mess. The CsoundQt build system needs some work and then this should be brought into appveyor.yaml.
cd staging\CsoundQt
echo "INCLUDE:" %INCLUDE%
set INCLUDE=%INCLUDE%%VCPKGDir%\\installed\\x64-windows\\include;
echo "INCLUDE:" %INCLUDE%
echo "CSOUND_LIBRARY_DIR:" %~dp0csound-vs\Release
dir %~dp0csound-vs\Release
qmake.exe qcs.pro ^
    CONFIG+=html_webengine ^
    CONFIG+=thread ^
    CONFIG+=csound6 ^
    CONFIG+=build64 ^
    CONFIG+=QCS_QT59 ^
    QT+=xml ^
    DEFINES+="USE_DOUBLE=1" ^
    DEFINES+="USE_QT_GT_58=1" ^
    CSOUND_API_INCLUDE_DIR="%APPVEYOR_BUILD_FOLDER%\\include" ^
    CSOUND_INTERFACES_INCLUDE_DIR="%APPVEYOR_BUILD_FOLDER%\\interfaces" ^
    CSOUND_LIBRARY_DIR="%APPVEYOR_BUILD_FOLDER%\\msvc\\csound-vs\\Release" ^
    DEFAULT_CSOUND_LIBS=csound64.lib ^
    CSOUND_LIBRARY=csound64.lib ^
    -after ^
    LCSOUND=%APPVEYOR_BUILD_FOLDER%\\msvc\\csound-vs\\Release\\csound64.lib ^
    LIBS+="%VCPKGDir%\\installed\\x64-windows\\lib\\libsndfile-1.lib" ^
    LIBS+="%APPVEYOR_BUILD_FOLDER%\\msvc\\csound-vs\\Release\\csound64.lib" ^
    CSOUND_INTERFACES_INCLUDE_DIR+="%~dp0deps\\include" ^
    CSOUND_INTERFACES_INCLUDE_DIR+="%APPVEYOR_BUILD_FOLDER%\\deps\\include" ^
    CSOUND_INTERFACES_INCLUDE_DIR+="%VCPKGDir%\\installed\\x64-windows\\include" ^
    INCLUDEPATH+="%VCPKGDir%\\installed\\x64-windows\\include"
nmake.exe
copy "bin\\CsoundQt*.*" "..\\..\\csound-vs\\Release\\"
cd ..\..
