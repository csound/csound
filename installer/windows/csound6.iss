  ; C S O U N D   6   I N N O   S E T U P   S C R I P T

; Copyright (C) 2013 by Michael Gogins.
; This software is licensed under the terms of the 
; Lesser (or Library) GNU Public License.

; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

; This file assumes that you are Michael Gogins,
; but comments indicate what you should change if you are not me.
; All of these changes SHOULD be in the #defines immediately following this.
; Also, this installer assumes LuaJIT and CsoundAC are part of Csound core.
; At this time the following features are not included in the installer:
; faustgen.
; I hope to change this soon.

; Uncomment the following line to build for Cabbage, CsoundVST, and vst4cs.
#define CSOUNDVST

#define MyAppName "Csound6"
#define MyAppVersion "6"
#ifdef CSOUNDVST
#define MyAppMinVersion "6.05.3beta-vst"
#else
#define MyAppMinVersion "6.05.3beta"
#endif
#define MyAppPublisher "Csound"
#define MyAppURL "http://sourceforge.net/projects/csound"
; If you are not Michael Gogins, change this to your MinGW dll directory.
#define MyMinGwLibDir "D:\Qt\Qt5.4.1\Tools\mingw491_32\i686-w64-mingw32\lib\"
; If you are not Michael Gogins, change this to your MSys /bin/ directory.
#define MyMSysBinDir "D:\msys\bin\"
; If you are not Michael Gogins, change this to your MSys /usr/local/ directory.
#define MyMSysUsrLocalDir "D:\msys\local\"
; If you are not Michael Gogins, change this to your Csound build directory.
#define MySourceDir "C:\Users\mike\csound-csound6-git\"
; If you are not Michael Gogins, change this to your Csound reference manual build directory.
#define MyManualSourceDir "C:\Users\mike\csound-manual6-git\"
; If you are not Michael Gogins, change this to your Csound tutorial directory.
#define MyCsoundTutorialSourceDir "D:\Dropbox\tutorial\"
; If you are not Michael Gogins, change this to your CsoundAC tutorial directory.
#define MyCsoundAcTutorialSourceDir "D:\Dropbox\tutorial\"
; If you are not Michael Gogins, change this to your libsndfile install directory.
#define MyLibSndfileSourceDir "D:\msys\local\opt\libsndfile\"
; If you are not Michael Gogins, change this to your PortAudio lib directory.
#define MyPortAudioSourceDir "D:\msys\local\src\portaudio\"
; If you are not Michael Gogins, change this to your PortMidi build directory.
#define MyPortMidiSourceDir "D:\msys\local\src\portmidi\"
; If you are not Michael Gogins, change this to your LuaJIT bin directory.
#define MyLuaJitSourceDir "D:\msys\local\src\luajit-2.0\src\"
; If you are not Michael Gogins, change this to your FLTK dll directory.
#define MyFltkSourceDir "D:\msys\local\src\fltk-1.3.2\src\"
; If you are not Michael Gogins, change this to your libmusicxml dll directory.
#define MyLibMusicXmlSourceDir "D:\msys\local\src\libmusicxml\"
; If you are not Michael Gogins, change this to your FluidSynth dll directory.
#define MyFluidSynthSourceDir "D:\msys\local\src\fluidsynth\src\.libs\"
; If you are not Michael Gogins, change this to your OSC library dll directory.
#define MyLibLoSourceDir "D:\msys\local\src\liblo-0.26\"
; If you are not Michael Gogins, change this to your STK dll directory.
#define MyLibStkSourceDir "D:\msys\local\src\stk-4.5.0\"
; If you are not Michael Gogins, change this to your CsoundQt repository directory.
#define MyCsoundQtDir "C:\Users\mike\qutecsound-code\"
; If you are not Michael Gogins, change this to your CsoundQt bin directory.
#define MyCsoundQtBinDir "C:\Users\mike\build-qcs-Desktop_Qt_5_4_0_MSVC2013_32bit-Release\bin\"
; If you are not Michael Gogins, change this to your Qt SDK DLL directory.
#define MyQtSdkBinDir "D:\Qt5.4.0\5.4\msvc2013\bin\"
; If you are not Michael Gogins, change this to your unzipped cabbage-master directory.
#define MyCabbageDir "D:\cabbage-master\"
; If you are not Michael Gogins, change this to your unzipped Chromium Embedded Framework directory.
#define MyCefHome "D:\msys\local\opt\cef_binary_3.2171.1979_windows32\"

[Components]
Name: "core"; Description: "Core Csound"; Types: full custom; Flags: fixed
Name: "python"; Description: "Python features (requires Python 2.7)"; Types: full; 
Name: "pd"; Description: "Pure Data csound~ object (requires Pure Data)"; Types: full; 
#ifdef CSOUNDVST
Name: "csoundvst"; Description: "Cabbage, Csound VST plugin, and vst4cs opcodes"; Types: full; 
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
Name: "{app}\cabbage"
#define APP_CABBAGE "{app}\cabbage\"

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
ChangesEnvironment=true
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{4BCEDC5F-C6D2-4869-9097-3F1753739F35}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppMinVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\{#MyAppName}
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
Source: "{#MyMinGwLibDir}*.dll"; DestDir: "{#APP_BIN}"; Components: core;
; No idea why this other name is needed.
Source: "{#MyMSysBinDir}libiconv-2.dll"; DestDir: "{#APP_BIN}"; DestName: "iconv.dll"; Components: core;
Source: "{#MyQtSdkBinDir}icuin53.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}icuuc53.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}icudt53.dll"; DestDir: "{#APP_BIN}"; Components: core;
 
; Microsoft C runtime library.
Source: "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\redist\x86\Microsoft.VC120.CRT\msvcr120.dll"; DestDir: "{#APP_BIN}"; Components: core;
; PThreads wants this.
Source: "C:\Windows\system32\msvcr100.dll"; DestDir: "{#APP_BIN}"; Components: core;
; Microsoft C++ runtime library.
Source: "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\redist\x86\Microsoft.VC120.CRT\msvcp120.dll"; DestDir: "{#APP_BIN}"; Components: core;

;Source: "{#MyMinGwLibDir}libgomp-1.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyMSysUsrLocalDir}bin/*.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "csound64.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "csnd6.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "*.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "*.lib"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "*.jar"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "*.pyd"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: python;
Source: "*.py";  DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: python;
Source: "frontends\icsound\*.py";  DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: python;
Source: "csound6~.dll";  DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: pd;
Source: "CsoundAC.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
#ifdef CSOUNDVST
Source: "CsoundVST.dll";  DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: csoundvst;
#endif
Source: "luaCsnd6.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "luaCsoundAC.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "_jcsound6.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "frontends\nwjs\build\Release\csound.node"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;

#ifdef CSOUNDVST
Source: "{#MyCabbageDir}cabbage.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: csoundvst
Source: "{#MyCabbageDir}CabbagePluginSynth.dat"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: csoundvst
Source: "{#MyCabbageDir}CabbagePluginEffect.dat"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: csoundvst
Source: "{#MyCabbageDir}Examples\*"; DestDir: "{#APP_BIN}\Examples"; Flags: ignoreversion recursesubdirs; Components: csoundvst
Source: "{#MyCabbageDir}Docs\*"; DestDir: "{#APP_BIN}\Docs"; Flags: ignoreversion recursesubdirs; Components: csoundvst
#endif

Source: "LuaCsound.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "ampmidid.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "buchla.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "cellular.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "chua.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "cs_date.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "csladspa.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "doppler.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "exciter.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "fareygen.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "fluidOpcodes.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "fractalnoise.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "image.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "ipmidi.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "linear_algebra.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "mixer.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "osc.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "padsynth.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "platerev.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "pmidi.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "py.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: python;
Source: "rtpa.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "rtwinmm.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "scansyn.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "serial.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "signalflowgraph.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "stdutil.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "stk.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "system_call.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "virtual.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
#ifdef CSOUNDVST
Source: "vst4cs.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: csoundvst;
#endif
Source: "widgets.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;

Source: "{#MyCsoundQtBinDir}CsoundQt-d-cs6.exe"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyCsoundQtDir}Examples\*.*"; DestDir: "{#APP_BIN}\Examples"; Flags: ignoreversion recursesubdirs;  Components: core 
Source: "{#MyQtSdkBinDir}Qt5Core.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5Gui.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5Network.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5PrintSupport.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5Qml.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5Quick.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5QuickWidgets.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5Widgets.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "{#MyQtSdkBinDir}Qt5Xml.dll"; DestDir: "{#APP_BIN}"; Components: core;
;Source: "{#MyQtSdkBinDir}..\plugins\accessible\qtaccessiblewidgets.dll"; DestDir: "{#APP_BIN}\plugins\accessible"; Components: core;
Source: "{#MyQtSdkBinDir}..\plugins\imageformats\*.dll"; DestDir: "{#APP_BIN}\plugins\imageformats"; Components: core;
Source: "{#MyQtSdkBinDir}..\plugins\platforms\qwindows.dll"; DestDir: "{#APP_BIN}\platforms"; Components: core;
Source: "{#MyQtSdkBinDir}..\plugins\printsupport\windowsprintersupport.dll"; DestDir: "{#APP_BIN}\plugins\printsupport"; Components: core;
Source: "D:\msys\local\opt\pthreads-w32-2-9-1-release\Pre-built.2\dll\x86\pthreadVC2.dll"; DestDir: "{#APP_BIN}"; Components: core;  
Source: "{#MyCefHome}Resources\*.*"; DestDir: "{#APP_BIN}"; Flags: ignoreversion recursesubdirs; Components: core;   
Source: "{#MyCefHome}Release\*.*"; DestDir: "{#APP_BIN}"; Excludes: "*.lib"; Flags: ignoreversion; Components: core;   

Source: {#MyLibSndfileSourceDir}\bin\*.*; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: {#MyLibSndfileSourceDir}\include\*.*; DestDir: "{#APP_INCLUDE}\sndfile"; Flags: ignoreversion; Components: core;

; Ignore the unspeakably stupid libtool crap.
; Source: {#MyPortAudioSourceDir}\lib\.libs\*.dll; DestDir: "{#APP_BIN}"; Components: core 
Source: {#MyPortAudioSourceDir}\bin\pa_devs.exe; DestDir: "{#APP_BIN}"; Components: core  
Source: {#MyPortAudioSourceDir}\bin\pa_minlat.exe; DestDir: "{#APP_BIN}"; Components: core  
; Required by pre-built portaudio_x86.dll (built with MSVC).
Source: "C:\Windows\system32\msvcr110.dll"; DestDir: "{#APP_BIN}"; Components: core  

Source: {#MyPortMidiSourceDir}*.dll; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core 

Source: {#MyLuaJitSourceDir}luajit.exe; DestDir: "{#APP_BIN}"; Flags: ignoreversion;  Components: core 

Source: {#MyLuaJitSourceDir}lua51.dll; DestDir: "{#APP_BIN}"; Flags: ignoreversion;  Components: core 
; No idea why this other name is needed.
Source: {#MyLuaJitSourceDir}lua51.dll; DestDir: "{#APP_BIN}"; DestName: "lua5.1.dll"; Flags: ignoreversion;  Components: core 
Source: {#MyLuaJitSourceDir}lua51.dll; DestDir: "{#APP_BIN}"; DestName: "luajit.dll"; Flags: ignoreversion;  Components: core 
Source: {#MyLuaJitSourceDir}lua*.h; DestDir: "{#APP_INCLUDE}\luajit"; Flags: ignoreversion;  Components: core 

Source: {#MyFltkSourceDir}*.dll; DestDir: "{#APP_BIN}"; Flags: ignoreversion;  Components: core 

Source: {#MyLibMusicXmlSourceDir}\cmake\*.dll; DestDir: "{#APP_BIN}"; Flags: ignoreversion;  Components: core 

; Source: {#MyFluidSynthSourceDir}*.dll; DestDir: "{#APP_BIN}"; Flags: ignoreversion;  Components: core 

; Source: {#MyLibLoSourceDir}*.dll; DestDir: "{#APP_BIN}"; Flags: ignoreversion;  Components: core 

Source: include/*.h*; DestDir: "{#APP_INCLUDE}\csound"; Flags: ignoreversion;  Components: core 
Source: interfaces/*.h*; DestDir: "{#APP_INCLUDE}\csound"; Flags: ignoreversion;  Components: core 
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
Name: "{group}\{cm:ProgramOnTheWeb,{#MyAppName}}"; Filename: "{#MyAppURL}";  Components: core;  
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{group}\Csound"; Filename: "cmd.exe"; Parameters: "/K csound.exe"; WorkingDir: "{#APP_BIN}"; Flags: dontcloseonexit;  Components: core  
Name: "{group}\CsoundQt"; Filename: "{#APP_BIN}CsoundQt-d-cs6.exe"; WorkingDir: "{#APP_BIN}";  Components: core   
#ifdef CSOUNDVST
Name: "{group}\Cabbage"; Filename: "{#APP_BIN}cabbage.exe"; WorkingDir: "{#APP_BIN}";  Components: csoundvst
#endif
Name: "{group}\LuaJIT"; Filename: "{#APP_BIN}luajit.exe"; WorkingDir: "{#APP_BIN}";  Components: core 
Name: "{group}\Audio device information"; Filename: "cmd"; Parameters: "/K pa_devs.exe"; WorkingDir: "{#APP_BIN}"; Flags: dontcloseonexit;  Components: core 
Name: "{group}\Audio device latency"; Filename: "cmd"; Parameters: "/K pa_minlat.exe"; WorkingDir: "{#APP_BIN}"; Flags: dontcloseonexit;  Components: core 
Name: "{group}\Csound Reference Manual"; Filename: "{#APP_MANUAL}indexframes.html";  Components: core  
Name: "{group}\Csound API Reference Manual"; Filename: "{#APP_APIREF}csound\index.html";  Components: core  
Name: "{group}\CsoundAC API Reference Manual"; Filename: "{#APP_APIREF}csoundac\index.html";  Components: core  
Name: "{group}\Csound Tutorial"; Filename: "{#APP_TUTORIAL}tutorial.pdf";  Components: core  
Name: "{group}\CsoundAC Tutorial"; Filename: "{#APP_TUTORIAL}Csound_Algorithmic_Composition_Tutorial.pdf";  Components: core  

[Registry]
Root: HKCU; Subkey: "Environment"; ValueType:string; ValueName:"RAWWAVE_PATH"; ValueData:"{#APP_SAMPLES}"; Flags: preservestringtype uninsdeletevalue;  Components: core 
Root: HKCU; Subkey: "Environment"; ValueType:string; ValueName:"OPCODE6DIR64"; ValueData:"{#APP_PLUGINS64}"; Flags: preservestringtype uninsdeletevalue;  Components: core 
Root: HKCU; Subkey: "Environment"; ValueType:string; ValueName:"PYTHONPATH"; ValueData:"{#APP_BIN}"; Flags: preservestringtype uninsdeletevalue;  Components: python 

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

