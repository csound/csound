cd staging\CsoundQt
echo "CSOUND_LIBRARY_DIR:" %~dp0csound-vs\Release
dir %~dp0csound-vs\Release
qmake.exe qcs.pro ^
    CONFIG+=html_webengine ^
    CONFIG+=perfThread_build ^
    QT+=xml ^
    DEFINES+="USE_DOUBLE=1" ^
    CSOUND_API_INCLUDE_DIR="%APPVEYOR_BUILD_FOLDER%\include" ^
    CSOUND_INTERFACES_INCLUDE_DIR="%APPVEYOR_BUILD_FOLDER%\interfaces" ^
    CSOUND_LIBRARY_DIR=%APPVEYOR_BUILD_FOLDER%\msvc\csound-vs\Release\ ^
    DEFAULT_CSOUND_LIBS=csound64.lib ^
    CSOUND_LIBRARY=csound64.lib ^
    LIBS+="%VCPKGDir%\installed\x64-windows\lib\libsndfile-1.lib" ^
    INCLUDEPATH+="%~dp0deps\include" ^
    INCLUDEPATH+="%VCPKGDir%\installed\x64-windows\include\"
nmake.exe
move "bin\CsoundQt*.*" "../../csound-vs/Release/"
cd ..\..
