; C S O U N D   6   I N N O   S E T U P   S C R I P T
;
; Copyright (C) 2013 by Michael Gogins.
; This software is licensed under the terms of the
; Lesser (or Library) GNU Public License.
;
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!
;
; This file is for building the Csound NSIS installer with AppVeyor and
; assumes that the build has been created by running "build.bat" from
; csound/msvc in the AppVeyor environment.
;
; DIRECTORY STRUCTURE
;
; C:\Program Files\Csound6
;     bin (copy Csound, CsoundQt, PortAudio, libsndfile, and NW.js binary trees here including runtime
;     libraries and dependency libraries; but not Csound opcodes)
;     doc (copy tree)
;     examples (copy tree but exclude .wav files)
;     include
;         csound (copy include and interfaces dirs *.h and *.hpp)
;         csoundac (copy dir *.hpp)
;     plugins64 (all Csound plugin opcodes)
;     samples (copy tree)
;
; USAGE
;
; Can run from the command line with "/S /D installation directory".
;
; Uncomment the following line to build CsoundVST and vst4cs:
; #define CSOUNDVST

#define AppName "Csound_x64"
#define AppVersion "6"
#ifdef CSOUNDVST
#define AppMinVersion "6.09.2beta3-vst"
#else
#define AppMinVersion "6.09.2beta3"
#endif
#define AppPublisher "Csound"
#define AppURL "http://csound.github.io/"
;#define QtSdkBinDir "C:\Qt\5.9.1\msvc2017_64\bin\"
#define LibStkSourceDir "..\..\msvc\deps\stk-master\"

[Components]
Name: "core"; Description: "Core Csound"; Types: full custom; Flags: fixed
Name: "python"; Description: "Python features (requires Python 2.7)"; Types: full;
#ifdef CSOUNDVST
Name: "vst"; Description: "Csound VST plugin and vst4cs opcodes"; Types: full;
#endif

[Dirs]
; ALL programs and shared libraries (except opcodes and other Csound modules) go here.
Name: "{app}\bin"; Permissions: users-modify
#define APP_BIN "{app}\bin\"
; ALL Csound opcodes and other Csound modules go here.
Name: "{app}\plugins64"; Permissions: users-modify
#define APP_PLUGINS64 "{app}\plugins64\"
; All C or C++ include files for Csound and other components used by Csound go here.
; This is a convenience for people like me who program in C or C++ and might use
; features of these third party components.
Name: "{app}\include"
#define APP_INCLUDE "{app}\include\"
; All Csound examples go here.
Name: "{app}\examples"; Permissions: users-modify
#define APP_EXAMPLES "{app}\examples\"
; Any SoundFonts or sound samples used by Csound examples go here.
Name: "{app}\samples"
#define APP_SAMPLES "{app}\samples\"
; Tutorials go here.
Name: "{app}\doc\tutorial"
#define APP_TUTORIAL "{app}\doc\tutorial\"

; These are the Csound environment variables related to directories.
#define SFDIR
#define SSDIR
#define SADIR
#define INCDIR
#define OPCODE6DIR64
#define SNAPDIR
#define RAWWAVE_PATH
#define MFDIR
#define PYTHONPATH

[Setup]
ChangesEnvironment=yes
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{180B4E5B-9A2F-4DA8-8692-97A174ACB74E}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppMinVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}
DefaultDirName={pf64}\{#AppName}
DefaultGroupName=Csound
AllowNoIcons=yes
LicenseFile=..\..\README.md
OutputDir=installer\windows
OutputBaseFilename=Setup_{#AppName}_{#AppMinVersion}
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "..\..\*.md"; DestDir: "{app}"; Flags: ignoreversion; Components: core;
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

; Microsoft C/C++ runtime libraries.
#define VCREDIST_CRT_DIR GetEnv("VCREDIST_CRT_DIR")
#define VCREDIST_CXXAMP_DIR GetEnv("VCREDIST_CXXAMP_DIR")
#define VCREDIST_OPENMP_DIR GetEnv("VCREDIST_OPENMP_DIR")

Source: "{#VCREDIST_CRT_DIR}\*"; DestDir: "{#APP_BIN}"; Flags: recursesubdirs; Components: core;
Source: "{#VCREDIST_CXXAMP_DIR}\*"; DestDir: "{#APP_BIN}"; Flags: recursesubdirs; Components: core;
Source: "{#VCREDIST_OPENMP_DIR}\*"; DestDir: "{#APP_BIN}"; Flags: recursesubdirs; Components: core;

Source: "..\..\emscripten\examples-wasm\httpd.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\emscripten\examples\httpd.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\emscripten\wasm\httpd.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\CsoundPerformanceTest.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\GeneralMidi.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\HelloWorld.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\Koch.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\Lindenmayer.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\Orbifold.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\PythonDemoApp.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\PythonDemoFrame.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\StrangeAtrractor.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\TrappedCsd.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\TrappedOrcSco.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\cb.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\display.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\drone.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\keys.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\messageCallback.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\oscilloscope.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\shapes.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\vu.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\examples\python\wxController.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\interfaces\ctcsound.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
#ifdef CSOUNDVST
Source: "..\..\msvc\csound-vs\Release\CsoundVST.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: vst;
#endif
Source: "..\..\msvc\csound-vs\Release\_csnd6.pyd"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: python;
Source: "..\..\msvc\csound-vs\Release\_jcsound6.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\ampmidid.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\arrayops.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\atsa.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\buchla.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\cellular.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\chua.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\cs.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\Release\cs_date.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\csanalyze.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\csb64enc.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\Release\csbeats.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\Release\csdebugger.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\Release\csladspa.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\csnd6.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\csnd6.jar"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\csnd6.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: python;
Source: "..\..\msvc\csound-vs\Release\csound.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\csound64.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
#ifdef CSOUNDVST
Source: "..\..\msvc\csound-vs\Release\csoundvstmain.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: vst;
#endif
Source: "..\..\frontends\nwjs\build\Release\csound.node"; DestDir: "{#APP_BIN}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\Release\cvanal.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\dnoise.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\doppler.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\emugens.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\envext.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\exciter.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\extract.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\extractor.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\fareygen.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\fluidOpcodes.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\fractalnoise.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\framebuffer.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\ftsamplebank.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\getftargs.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\hdf5ops.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\Release\het_export.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\het_import.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\hetro.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\image.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\ipmidi.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\linear_algebra.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\liveconv.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\lpanal.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\lpc_export.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\lpc_import.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\luaCsnd6.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\Release\makecsd.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\Release\mixer.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\mixer.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\osc.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\padsynth.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\platerev.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\pv_export.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\pv_import.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\pvanal.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\pvlook.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\pvsops.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\py.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: python;
Source: "..\..\msvc\csound-vs\Release\quadbezier.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\rtpa.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\Release\rtwinmm.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\scale.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\scansyn.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\scot.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\Release\scsort.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\Release\scugens.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\sdif2ad.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\select.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\serial.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\signalflowgraph.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\sndinfo.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\srconv.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\stackops.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\stdutil.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\stkops.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\system_call.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\Release\virtual.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
#ifdef CSOUNDVST
Source: "..\..\msvc\csound-vs\Release\vst4cs.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: vst;
#endif
Source: "..\..\msvc\csound-vs\Release\widgets.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\deps\bin\*.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\deps\fluidsynthdeps\bin\*.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;

; NOTE: The .qml files are compiled into the resources of CsoundQt.
Source: "..\..\msvc\csound-vs\Release\CsoundQt-d-html-cs6.exe"; DestDir: "{#APP_BIN}"; Flags: skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\staging\CsoundQt\examples\*.*"; DestDir: "{#APP_BIN}\Examples"; Flags: ignoreversion recursesubdirs;  Components: core
Source: "..\..\msvc\staging\CsoundQt\src\Examples\*.*"; DestDir: "{#APP_BIN}\Examples"; Flags: ignoreversion recursesubdirs;  Components: core

Source: "{#QtSdkBinDir}libEGL.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}libGLESv2.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5Core.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5Gui.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5Multimedia.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5MultimediaWidgets.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5Network.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5OpenGL.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5Positioning.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5PrintSupport.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5Qml.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5Quick.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5QuickWidgets.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5Sql.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5Svg.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5WebChannel.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5WebEngine.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5WebEngineCore.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}QtWebEngineProcess.exe"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5WebEngineWidgets.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5Widgets.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5Xml.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}Qt5XmlPatterns.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#QtSdkBinDir}..\plugins\imageformats\*.dll"; DestDir: "{#APP_BIN}\plugins\imageformats"; Components: core;
Source: "{#QtSdkBinDir}..\plugins\platforms\qwindows.dll"; DestDir: "{#APP_BIN}\platforms"; Components: core;
Source: "{#QtSdkBinDir}..\plugins\printsupport\windowsprintersupport.dll"; DestDir: "{#APP_BIN}\plugins\printsupport"; Components: core;
Source: "{#QtSdkBinDir}..\qml\QtQml\*.*"; DestDir: "{#APP_BIN}QtQml"; Flags: ignoreversion recursesubdirs;  Components: core;
Source: "{#QtSdkBinDir}..\qml\QtQuick\*.*"; DestDir: "{#APP_BIN}QtQuick"; Flags: ignoreversion recursesubdirs;  Components: core;
Source: "{#QtSdkBinDir}..\qml\QtQuick.2\*.*"; DestDir: "{#APP_BIN}QtQuick.2"; Flags: ignoreversion recursesubdirs;  Components: core;
Source: "{#QtSdkBinDir}..\resources\*.*"; DestDir: "{#APP_BIN}"; Flags: ignoreversion recursesubdirs;  Components: core;

Source: "{#VcpkgInstalledBinDir}*.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion recursesubdirs;  Components: core;

Source: "..\..\msvc\csound-vs\Release\libsndfile-1.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
;; Source: {#LibSndfileSourceDir}\include\*.*; DestDir: "{#APP_INCLUDE}\sndfile"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;

Source: ../../include/*.h*; DestDir: "{#APP_INCLUDE}\csound"; Flags: ignoreversion;  Components: core
Source: ../../H/pffft.h; DestDir: "{#APP_INCLUDE}\csound"; Flags: ignoreversion;  Components: core
Source: ../../interfaces/*.h*; DestDir: "{#APP_INCLUDE}\csound"; Flags: ignoreversion;  Components: core
Source: ../../interfaces/csPerfThread.*; DestDir: "{#APP_INCLUDE}\csound"; Flags: ignoreversion;  Components: core
Source: ../../interfaces/*.py; DestDir: "{#APP_BIN}"; Flags: ignoreversion;  Components: core
Source: ../../frontends/CsoundAC/*.hpp; DestDir: "{#APP_INCLUDE}\csoundac"; Flags: ignoreversion;  Components: core

Source: "../../examples\*.*"; DestDir: "{#APP_EXAMPLES}"; Excludes: "*.wav *.html"; Flags: ignoreversion recursesubdirs;  Components: core

Source: "../../samples\*.*"; DestDir: "{#APP_SAMPLES}"; Flags: ignoreversion recursesubdirs;  Components: core
Source: "{#LibStkSourceDir}rawwaves\*.*"; DestDir: "{#APP_SAMPLES}"; Flags: ignoreversion recursesubdirs;  Components: core

[Icons]
Name: "{group}\{cm:ProgramOnTheWeb,Csound}"; Filename: "{#AppURL}";  Components: core;
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
Name: "{group}\Csound"; Filename: "cmd.exe"; Parameters: "/K csound.exe"; WorkingDir: "{#APP_BIN}"; Flags: dontcloseonexit;  Components: core
Name: "{group}\CsoundQt"; Filename: "{#APP_BIN}CsoundQt-d-html-cs6.exe"; WorkingDir: "{#APP_BIN}";  Components: core
Name: "{group}\Csound Reference Manual"; Filename: "http://csound.github.io/docs/manual/indexframes.html";  Components: core
Name: "{group}\Csound API Reference Manual"; Filename: "http://csound.github.io/docs/api/index.html";  Components: core

[Registry]
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType:string; ValueName:"OPCODE6DIR64"; ValueData:"{#APP_PLUGINS64}"; Flags: preservestringtype uninsdeletevalue;  Components: core
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType:string; ValueName:"PYTHONPATH"; ValueData:"{#APP_BIN};{olddata}"; Flags: preservestringtype uninsdeletevalue;  Components: python
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType:string; ValueName:"RAWWAVE_PATH"; ValueData:"{#APP_SAMPLES}"; Flags: preservestringtype uninsdeletevalue;  Components: core

[Tasks]
Name: modifypath; Description: &Add application directory to your PATH environment variable; Components: core;

[Code]
//	ModPathName defines the name of the task defined above
//	ModPathType defines whether the 'user' or 'system' path will be modified;
//		this will default to user if anything other than system is set
//	setArrayLength must specify the total number of dirs to be added
//	Result[0] contains first directory, Result[1] contains second, etc.
const ModPathName = 'modifypath';
ModPathType = 'system';

function ModPathDir(): TArrayOfString;
begin
	setArrayLength(Result, 1);
	Result[0] := ExpandConstant('{app}\bin');
end;

procedure ModPath();
var
	oldpath:	String;
	newpath:	String;
	updatepath:	Boolean;
	pathArr:	TArrayOfString;
	aExecFile:	String;
	aExecArr:	TArrayOfString;
	i, d:		Integer;
	pathdir:	TArrayOfString;
	regroot:	Integer;
	regpath:	String;
begin
	// Get constants from main script and adjust behavior accordingly
	// ModPathType MUST be 'system' or 'user'; force 'user' if invalid
	if ModPathType = 'system' then begin
		regroot := HKEY_LOCAL_MACHINE;
		regpath := 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment';
	end else begin
		regroot := HKEY_CURRENT_USER;
		regpath := 'Environment';
	end;

	// Get array of new directories and act on each individually
	pathdir := ModPathDir();
	for d := 0 to GetArrayLength(pathdir)-1 do begin
		updatepath := true;

		// Modify WinNT path
		if UsingWinNT() = true then begin

			// Get current path, split into an array
			RegQueryStringValue(regroot, regpath, 'Path', oldpath);
			oldpath := oldpath + ';';
			i := 0;

			while (Pos(';', oldpath) > 0) do begin
				SetArrayLength(pathArr, i+1);
				pathArr[i] := Copy(oldpath, 0, Pos(';', oldpath)-1);
				oldpath := Copy(oldpath, Pos(';', oldpath)+1, Length(oldpath));
				i := i + 1;

				// Check if current directory matches app dir
				if pathdir[d] = pathArr[i-1] then begin
					// if uninstalling, remove dir from path
					if IsUninstaller() = true then begin
						continue;
					// if installing, flag that dir already exists in path
					end else begin
						updatepath := false;
					end;
				end;

				// Add current directory to new path
				if i = 1 then begin
					newpath := pathArr[i-1];
				end else begin
					newpath := newpath + ';' + pathArr[i-1];
				end;
			end;

			// Append app dir to path if not already included
			if (IsUninstaller() = false) AND (updatepath = true) then
				newpath := newpath + ';' + pathdir[d];

			// Write new path
			RegWriteStringValue(regroot, regpath, 'Path', newpath);

		// Modify Win9x path
		end else begin

			// Convert to shortened dirname
			pathdir[d] := GetShortName(pathdir[d]);

			// If autoexec.bat exists, check if app dir already exists in path
			aExecFile := 'C:\AUTOEXEC.BAT';
			if FileExists(aExecFile) then begin
				LoadStringsFromFile(aExecFile, aExecArr);
				for i := 0 to GetArrayLength(aExecArr)-1 do begin
					if IsUninstaller() = false then begin
						// If app dir already exists while installing, skip add
						if (Pos(pathdir[d], aExecArr[i]) > 0) then
							updatepath := false;
							break;
					end else begin
						// If app dir exists and = what we originally set, then delete at uninstall
						if aExecArr[i] = 'SET PATH=%PATH%;' + pathdir[d] then
							aExecArr[i] := '';
					end;
				end;
			end;

			// If app dir not found, or autoexec.bat didn't exist, then (create and) append to current path
			if (IsUninstaller() = false) AND (updatepath = true) then begin
				SaveStringToFile(aExecFile, #13#10 + 'SET PATH=%PATH%;' + pathdir[d], True);

			// If uninstalling, write the full autoexec out
			end else begin
				SaveStringsToFile(aExecFile, aExecArr, False);
			end;
		end;
	end;
end;

// Split a string into an array using passed delimeter
procedure MPExplode(var Dest: TArrayOfString; Text: String; Separator: String);
var
	i: Integer;
begin
	i := 0;
	repeat
		SetArrayLength(Dest, i+1);
		if Pos(Separator,Text) > 0 then	begin
			Dest[i] := Copy(Text, 1, Pos(Separator, Text)-1);
			Text := Copy(Text, Pos(Separator,Text) + Length(Separator), Length(Text));
			i := i + 1;
		end else begin
			 Dest[i] := Text;
			 Text := '';
		end;
	until Length(Text)=0;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
	taskname:	String;
begin
	taskname := ModPathName;
	if CurStep = ssPostInstall then
		if IsTaskSelected(taskname) then
			ModPath();
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
	aSelectedTasks:	TArrayOfString;
	i:				Integer;
	taskname:		String;
	regpath:		String;
	regstring:		String;
	appid:			String;
begin
	// only run during actual uninstall
	if CurUninstallStep = usUninstall then begin
		// get list of selected tasks saved in registry at install time
		appid := '{#emit SetupSetting("AppId")}';
		if appid = '' then appid := '{#emit SetupSetting("AppName")}';
		regpath := ExpandConstant('Software\Microsoft\Windows\CurrentVersion\Uninstall\'+appid+'_is1');
		RegQueryStringValue(HKLM, regpath, 'Inno Setup: Selected Tasks', regstring);
		if regstring = '' then RegQueryStringValue(HKCU, regpath, 'Inno Setup: Selected Tasks', regstring);

		// check each task; if matches modpath taskname, trigger patch removal
		if regstring <> '' then begin
			taskname := ModPathName;
			MPExplode(aSelectedTasks, regstring, ',');
			if GetArrayLength(aSelectedTasks) > 0 then begin
				for i := 0 to GetArrayLength(aSelectedTasks)-1 do begin
					if comparetext(aSelectedTasks[i], taskname) = 0 then
						ModPath();
				end;
			end;
		end;
	end;
end;

function NeedRestart(): Boolean;
var
	taskname:	String;
begin
	taskname := ModPathName;
	if IsTaskSelected(taskname) and not UsingWinNT() then begin
		Result := True;
	end else begin
		Result := False;
	end;
end;

