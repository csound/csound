cd staging\CsoundQt
qmake.exe qcs.pro -spec win32-msvc2015 ^
    CONFIG+=html_webengine ^
    CONFIG+=perfThread_build ^
    CSOUND_API_INCLUDE_DIR=%~dp0/"../include" ^
    CSOUND_INTERFACES_INCLUDE_DIR=%~dp0/"../interfaces" ^
    CSOUND_LIBRARY_DIR=%~dp0csound-vs\\Release ^
    LIBS+=%~dp0/deps/lib/libsndfile-1.lib ^
    INCLUDEPATH+="%~dp0deps\include"
nmake.exe
cd ..\..
