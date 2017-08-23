cd staging\CsoundQt
qmake.exe qcs.pro ^
    CONFIG+=html_webengine ^
    CONFIG+=perfThread_build ^
    QT+=xml ^
    DEFINES+="USE_DOUBLE=1" ^
    CSOUND_API_INCLUDE_DIR=%~dp0/"../include" ^
    CSOUND_INTERFACES_INCLUDE_DIR=%~dp0/"../interfaces" ^
    CSOUND_LIBRARY_DIR=%~dp0csound-vs/Release ^
    CSOUND_LIBRARY=csound64.lib ^
    LIBS+=%~dp0/deps/lib/libsndfile-1.lib ^
    INCLUDEPATH+="%~dp0deps\include"
nmake.exe   
cd ..\..
