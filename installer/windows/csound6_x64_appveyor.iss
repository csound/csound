; C S O U N D   6   N U L L S O F T   S E T U P   S C R I P T
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
;     bin (copy Csound, CsoundQt, PortAudio, libsndfile, LuaJIT, and NW.js binary trees here; but not Csound opcodes).
;     doc (copy tree)
;     examples (copy tree but exclude .wav files)
;     include
;         csound (copy include and interfaces dirs *.h and *.hpp)
;         csoundac (copy dir *.hpp)
;         luajit (copy dir *.h)
;         sndfile (copy dir *.h)
;     plugins64 (all Csound plugin opcodes)
;     samples (copy tree)
;
; At this time the following features are not included in the installer:
; faustgen, csound~.
;
; USAGE
;
; Can run from the command line with "/S /D installation directory".
;
; Uncomment the following line to build CsoundVST and vst4cs:
; #define CSOUNDVST

#define MyAppName "Csound6_x64"
#define MyAppVersion "6"
#ifdef CSOUNDVST
#define MyAppMinVersion "6.09.2beta3-vst"
#else
#define MyAppMinVersion "6.09.2beta3"
#endif
#define MyAppPublisher "Csound"
#define MyAppURL "http://csound.github.io/"
; If you are not Michael Gogins, change this to your msys64 directory.
#define MyMsys64Dir "D:\msys64\"
; If you are not Michael Gogins, change this to your mingw64 directory.
#define MyMingw64Dir "D:\msys64\mingw64\"
; If you are not Michael Gogins, change this to your Csound build directory.
#define MySourceDir "D:\msys64\home\restore\csound\"
; If you are not Michael Gogins, change this to your Csound reference manual build directory.
#define MyManualSourceDir "D:\msys64\home\restore\manual\"
; If you are not Michael Gogins, change this to your Csound tutorial directory.
#define MyCsoundTutorialSourceDir "D:\Dropbox\tutorial\"
; If you are not Michael Gogins, change this to your CsoundAC tutorial directory.
#define MyCsoundAcTutorialSourceDir "D:\Dropbox\tutorial\"
; If you are not Michael Gogins, change this to your libsndfile install directory.
#define MyLibSndfileSourceDir "C:\Program_Files\Mega-Nerd\libsndfile\"
; If you are not Michael Gogins, change this to your CsoundQt repository directory.
#define MyCsoundQtDir "C:\Users\restore\CsoundQt\"
; If you are not Michael Gogins, change this to your CsoundQt bin directory.
#define MyCsoundQtBinDir "C:\Users\restore\CsoundQt\bin\"
; If you are not Michael Gogins, change this to your Qt SDK directory.
#define MyQtSdkDir "C:\Qt\"
; If you are not Michael Gogins, change this to your Qt SDK DLL directory.
#define MyQtSdkBinDir "C:\Qt\Qt5.8.0\5.8\msvc2015_64\bin\"
; If you are not Michael Gogins, change this to your PythonQt DLL directory.
#define MyPythonQtBinDir "C:\Users\restore\PythonQt3.1_tarmo\lib\"
; If you are not Michael Gogins, change this to your STK source directory.
#define MyLibStkSourceDir "D:\msys64\home\restore\csound\mingw64\packages\stk\src\stk-4.5.1\"
; If you are not Michael Gogins, change this to your NW.js installation directory.
#define MyNwJsDir "D:\nwjs-sdk-v0.23.5-win-x64\"
; If you are not Michael Gogins, change this to your Winpthreads installation directory.
#define MyWinPthreadsDir "D:\msys64\home\restore\pthreads-w32-2-9-1-release\Pre-built.2\"
#define MyPackagesDir "D:\msys64\home\restore\csound\mingw64\packages\"

[Components]
Name: "core"; Description: "Core Csound"; Types: full custom; Flags: fixed
Name: "python"; Description: "Python features (requires Python 2.7)"; Types: full;
#ifdef CSOUNDVST
Name: "csoundvst"; Description: "Csound VST plugin and vst4cs opcodes"; Types: full;
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
; The Csound reference manual goes here.
Name: "{app}\doc\manual"
#define APP_MANUAL "{app}\doc\manual\"
; The Csound API reference manuals go here.
Name: "{app}\doc\apiref"
#define APP_APIREF "{app}\doc\apiref\"
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
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppMinVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf64}\{#MyAppName}
DefaultGroupName=Csound 6
AllowNoIcons=yes
LicenseFile=README.md
;InfoBeforeFile=readme-csound6.txt
OutputDir=installer\windows
OutputBaseFilename=Setup_{#MyAppName}_{#MyAppMinVersion}
Compression=lzma
SolidCompression=yes
SourceDir={#MySourceDir}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "*.md"; DestDir: "{app}"; Flags: ignoreversion; Components: core;
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

; Microsoft C/C++ runtime libraries.
Source: "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\redist\x64\Microsoft.VC120.CRT\*"; DestDir: "{#APP_BIN}"; Flags: recursesubdirs; Components: core;
Source: "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x64\Microsoft.VC140.CRT\*"; DestDir: "{#APP_BIN}"; Flags: recursesubdirs; Components: core;

Source: "D:\msys64\home\restore\csound\emscripten\examples-wasm\httpd.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\emscripten\examples\httpd.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\emscripten\wasm\httpd.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\CsoundPerformanceTest.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\GeneralMidi.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\HelloWorld.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\Koch.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\Lindenmayer.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\Orbifold.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\PythonDemoApp.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\PythonDemoFrame.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\StrangeAtrractor.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\TrappedCsd.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\TrappedOrcSco.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\cb.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\display.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\drone.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\keys.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\messageCallback.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\oscilloscope.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\shapes.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\vu.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\examples\python\wxController.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\interfaces\ctcsound.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
#ifdef CSOUNDVST
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\CsoundVST.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: csoundvst;
#endif
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\LuaCsound.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\_csnd6.pyd"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: python;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\_jcsound6.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\ampmidid.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\arrayops.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\atsa.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\buchla.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\cellular.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\chua.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\cs.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\cs_date.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\csanalyze.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\csb64enc.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\csbeats.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\csdebugger.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\csladspa.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\csnd6.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\csnd6.jar"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\csnd6.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: python;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\csound.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\csound64.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
#ifdef CSOUNDVST
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\csoundvstmain.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: csoundvst;
#endif
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\cvanal.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\dnoise.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\doppler.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\emugens.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\envext.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\exciter.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\extract.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\extractor.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\fareygen.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\fluidOpcodes.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\fractalnoise.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\framebuffer.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\ftsamplebank.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\getftargs.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\hdf5ops.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\het_export.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\het_import.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\hetro.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\image.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\ipmidi.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\linear_algebra.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\liveconv.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\lpanal.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\lpc_export.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\lpc_import.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\luaCsnd6.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\makecsd.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\mixer.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\mixer.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\osc.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\padsynth.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\platerev.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\pv_export.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\pv_import.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\pvanal.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\pvlook.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\pvsops.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\py.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: python;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\quadbezier.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\rtpa.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\rtwinmm.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\scale.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\scansyn.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\scot.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\scsort.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\scugens.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\sdif2ad.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\select.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\serial.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\signalflowgraph.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\sndinfo.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\srconv.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\stackops.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\stdutil.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\stkops.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\system_call.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\virtual.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
#ifdef CSOUNDVST
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\vst4cs.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: csoundvst;
#endif
Source: "D:\msys64\home\restore\csound\mingw64\csound-mingw64\widgets.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\packages\stk\pkg\mingw-w64-x86_64-stk-devel\mingw64\lib\libstk-4.5.1.so"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\packages\stk\pkg\mingw-w64-x86_64-stk-devel\mingw64\lib\libstk.so"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\packages\stk\pkg\mingw-w64-x86_64-stk\mingw64\bin\libstk-4.5.1.so"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\packages\stk\pkg\mingw-w64-x86_64-stk\mingw64\bin\libstk.so"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\packages\stk\src\dest\mingw64\lib\libstk-4.5.1.so"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\packages\stk\src\dest\mingw64\lib\libstk.so"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\packages\stk\src\stk-4.5.1\src\libstk-4.5.1.so"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\home\restore\csound\mingw64\packages\stk\src\stk-4.5.1\src\libstk.so"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\mingw64\bin\libFLAC-8.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\mingw64\bin\libgcc_s_seh-1.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\mingw64\bin\libglib-2.0-0.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\mingw64\bin\libiconv-2.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\mingw64\bin\libintl-8.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\mingw64\bin\libogg-0.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\mingw64\bin\libpcre-1.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\mingw64\bin\libportaudio-2.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\mingw64\bin\libsndfile-1.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\mingw64\bin\libspeex-1.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\mingw64\bin\libstdc++-6.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\mingw64\bin\libvorbis-0.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\mingw64\bin\libvorbisenc-2.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\mingw64\bin\libwinpthread-1.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "D:\msys64\mingw64\bin\lua51.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;

; Some targets not identified by "find_csound_dependencies.py."
Source: "{#MySourceDir}/mingw64/csound64.lib"; DestDir: "{app}\lib"; Components: core;
Source: "{#MySourceDir}frontends/nwjs/build/Release/csound.node"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MySourceDir}frontends/nwjs/build/Release/csound.pdb"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyNwJsDir}*.*"; DestDir: "{#APP_BIN}\"; Flags: ignoreversion recursesubdirs;  Components: core
Source: "{#MyPackagesDir}portaudio_asio\src\mingw-w64-x86_64-portaudio\bin\.libs\pa_devs.exe"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyPackagesDir}portaudio_asio\src\mingw-w64-x86_64-portaudio\bin\.libs\pa_minlat.exe"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MySourceDir}Opcodes\AbletonLinkOpcodes\x64\Release\*.dll"; DestDir: "{#APP_PLUGINS64}"; Components: core;

Source: "{#MyMingw64Dir}\bin\luajit.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;

; NOTE: The .qml files are compiled into the resources of CsoundQt.
Source: "{#MyCsoundQtBinDir}CsoundQt-d-html-cs6.exe"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyCsoundQtDir}examples\*.*"; DestDir: "{#APP_BIN}\Examples"; Flags: ignoreversion recursesubdirs;  Components: core
Source: "{#MyCsoundQtDir}src\Examples\*.*"; DestDir: "{#APP_BIN}\Examples"; Flags: ignoreversion recursesubdirs;  Components: core

;Source: "{#MyPythonQtBinDir}PythonQt.dll"; DestDir: "{#APP_BIN}"; Components: core;
;Source: "{#MyPythonQtBinDir}PythonQt_QtAll.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}libEGL.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}libGLESv2.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5Core.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5Gui.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5Multimedia.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5MultimediaWidgets.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5Network.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5OpenGL.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5Positioning.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5PrintSupport.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5Qml.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5Quick.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5QuickWidgets.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5Sql.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5Svg.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5WebChannel.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5WebEngine.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5WebEngineCore.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5WebEngineWidgets.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5Widgets.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5Xml.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5XmlPatterns.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}..\plugins\imageformats\*.dll"; DestDir: "{#APP_BIN}\plugins\imageformats"; Components: core;
Source: "{#MyQtSdkBinDir}..\plugins\platforms\qwindows.dll"; DestDir: "{#APP_BIN}\platforms"; Components: core;
Source: "{#MyQtSdkBinDir}..\plugins\printsupport\windowsprintersupport.dll"; DestDir: "{#APP_BIN}\plugins\printsupport"; Components: core;
Source: "{#MyQtSdkBinDir}..\qml\QtQml\*.*"; DestDir: "{#APP_BIN}QtQml"; Flags: ignoreversion recursesubdirs;  Components: core;
Source: "{#MyQtSdkBinDir}..\qml\QtQuick\*.*"; DestDir: "{#APP_BIN}QtQuick"; Flags: ignoreversion recursesubdirs;  Components: core;
Source: "{#MyQtSdkBinDir}..\qml\QtQuick.2\*.*"; DestDir: "{#APP_BIN}QtQuick.2"; Flags: ignoreversion recursesubdirs;  Components: core;

Source: {#MyWinPthreadsDir}dll\x64\pthreadVC2.dll; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;

Source: {#MyLibSndfileSourceDir}\bin\*.*; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: {#MyLibSndfileSourceDir}\include\*.*; DestDir: "{#APP_INCLUDE}\sndfile"; Flags: ignoreversion; Components: core;

Source: include/*.h*; DestDir: "{#APP_INCLUDE}\csound"; Flags: ignoreversion;  Components: core
Source: H/pffft.h; DestDir: "{#APP_INCLUDE}\csound"; Flags: ignoreversion;  Components: core
Source: interfaces/*.h*; DestDir: "{#APP_INCLUDE}\csound"; Flags: ignoreversion;  Components: core
Source: interfaces/csPerfThread.*; DestDir: "{#APP_INCLUDE}\csound"; Flags: ignoreversion;  Components: core
Source: interfaces/*.py; DestDir: "{#APP_BIN}"; Flags: ignoreversion;  Components: core
Source: frontends/CsoundAC/*.hpp; DestDir: "{#APP_INCLUDE}\csoundac"; Flags: ignoreversion;  Components: core

Source: {#MyManualSourceDir}html\*.*; DestDir: "{#APP_MANUAL}"; Flags: ignoreversion recursesubdirs;  Components: core

Source: "doc\doxygen\csound\html\*.*"; DestDir: "{#APP_APIREF}/csound"; Flags: ignoreversion recursesubdirs;  Components: core
Source: "doc\doxygen\csoundac\html\*.*"; DestDir: "{#APP_APIREF}/csoundac"; Flags: ignoreversion recursesubdirs;  Components: core

Source: "examples\*.*"; DestDir: "{#APP_EXAMPLES}"; Excludes: "*.wav *.html"; Flags: ignoreversion recursesubdirs;  Components: core

Source: "samples\*.*"; DestDir: "{#APP_SAMPLES}"; Flags: ignoreversion recursesubdirs;  Components: core
Source: "{#MyLibStkSourceDir}rawwaves\*.*"; DestDir: "{#APP_SAMPLES}"; Flags: ignoreversion recursesubdirs;  Components: core

Source: {#MyCsoundTutorialSourceDir}tutorial.pdf; DestDir: "{#APP_TUTORIAL}"; Flags: ignoreversion recursesubdirs;  Components: core
Source: {#MyCsoundAcTutorialSourceDir}Csound_Algorithmic_Composition_Tutorial.pdf; DestDir: "{#APP_TUTORIAL}"; Flags: ignoreversion recursesubdirs;  Components: core
Source: {#MyCsoundAcTutorialSourceDir}code\*.*; DestDir: "{#APP_TUTORIAL}code\"; Excludes: "*.wav"; Flags: ignoreversion recursesubdirs;  Components: core

Source: "doc\csound_system_documentation\*.pdf"; DestDir:"{#APP_APIREF}"; Flags: ignoreversion; Components: core

[Icons]
Name: "{group}\{cm:ProgramOnTheWeb,Csound}"; Filename: "{#MyAppURL}";  Components: core;
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{group}\Csound"; Filename: "cmd.exe"; Parameters: "/K csound.exe"; WorkingDir: "{#APP_BIN}"; Flags: dontcloseonexit;  Components: core
Name: "{group}\CsoundQt"; Filename: "{#APP_BIN}CsoundQt-d-html-cs6.exe"; WorkingDir: "{#APP_BIN}";  Components: core
Name: "{group}\LuaJIT"; Filename: "{#APP_BIN}luajit.exe"; WorkingDir: "{#APP_BIN}";  Components: core
Name: "{group}\Audio device information"; Filename: "cmd"; Parameters: "/K pa_devs.exe"; WorkingDir: "{#APP_BIN}"; Flags: dontcloseonexit;  Components: core
Name: "{group}\Audio device latency"; Filename: "cmd"; Parameters: "/K pa_minlat.exe"; WorkingDir: "{#APP_BIN}"; Flags: dontcloseonexit;  Components: core
Name: "{group}\Csound Reference Manual"; Filename: "{#APP_MANUAL}indexframes.html";  Components: core
Name: "{group}\Csound API Reference Manual"; Filename: "{#APP_APIREF}csound\index.html";  Components: core
Name: "{group}\CsoundAC API Reference Manual"; Filename: "{#APP_APIREF}csoundac\index.html";  Components: core
Name: "{group}\Csound Tutorial"; Filename: "{#APP_TUTORIAL}tutorial.pdf";  Components: core
Name: "{group}\CsoundAC Tutorial"; Filename: "{#APP_TUTORIAL}Csound_Algorithmic_Composition_Tutorial.pdf";  Components: core

[Registry]
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType:string; ValueName:"OPCODE6DIR64"; ValueData:"{#APP_PLUGINS64}"; Flags: preservestringtype uninsdeletevalue;  Components: core
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType:string; ValueName:"NODE_PATH"; ValueData:"{#APP_BIN};{olddata}"; Flags: preservestringtype uninsdeletevalue;  Components: core
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType:string; ValueName:"PYTHONPATH"; ValueData:"{#APP_BIN};{olddata}"; Flags: preservestringtype uninsdeletevalue;  Components: python
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType:string; ValueName:"RAWWAVE_PATH"; ValueData:"{#APP_SAMPLES}"; Flags: preservestringtype uninsdeletevalue;  Components: core
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType:string; ValueName:"LUA_PATH"; ValueData:"{#APP_EXAMPLES}lua\?.lua"; Flags: preservestringtype uninsdeletevalue;  Components: core
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType:string; ValueName:"LUA_CPATH"; ValueData:"{#APP_BIN}?.dll"; Flags: preservestringtype uninsdeletevalue;  Components: core

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

