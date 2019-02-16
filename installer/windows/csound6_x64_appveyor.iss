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
;     bin (copy Csound, PortAudio, libsndfile binary trees here including runtime
;     libraries and dependency libraries; but not Csound opcodes)
;     doc (copy tree)
;     examples (copy tree but exclude .wav files)
;     include
;         csound (copy include and interfaces dirs *.h and *.hpp)
;     plugins64 (all Csound plugin opcodes)
;     samples (copy tree)
;

[setup]
#define AppName "Csound6_x64"
#define AppVersion "6"
#define AppMinVersion GetEnv("CSOUND_VERSION")
#define AppveyorBuildNumber GetEnv("APPVEYOR_BUILD_NUMBER")
#define AppPublisher "Csound"
#define AppURL "http://csound.github.io/"
#define LibStkSourceDir "..\..\msvc\deps\stk-master\"
#define ManualSourceDir "c:\html\"
#define CsoundQTDir "c:\CsoundQt-0.9.5-win64\bin\"
#define CsoundQTIcon "c:\qtcs.ico"

[Components]
Name: "core"; Description: "Core Csound"; Types: full custom; Flags: fixed
Name: "csoundqt"; Description: "Csound QT"; Types: full;
Name: "python"; Description: "Python features (requires Python 2.7)"; Types: full;

[Dirs]
; ALL programs and shared libraries (except opcodes and other Csound modules) go here.
Name: "{app}\bin"; Permissions: users-modify
#define APP_BIN "{app}\bin\"
; ALL static and import libraries for the Csound and CsoundAC SDKs go here.
Name: "{app}\lib"; Permissions: users-modify
#define APP_LIB "{app}\lib\"
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

; Csound manual
Name: "{app}\doc\manual"
#define APP_MANUAL "{app}\doc\manual\"

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
;AppVerName={#AppName}-{AppMinVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}
DefaultDirName={pf64}\{#AppName}
DefaultGroupName=Csound
AllowNoIcons=yes
LicenseFile=..\..\README.md
OutputDir=installer\windows
OutputBaseFilename={#AppName}-{#AppMinVersion}-{#AppveyorBuildNumber}
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "..\..\*.md"; DestDir: "{app}"; Flags: ignoreversion; Components: core;
Source: "..\..\installer\windows\INSTALLER.md"; DestDir: "{app}"; Flags: ignoreversion; Components: core;
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

; Microsoft C/C++ runtime libraries.
#define VCREDIST_CRT_DIR GetEnv("VCREDIST_CRT_DIR")
#define VCREDIST_CXXAMP_DIR GetEnv("VCREDIST_CXXAMP_DIR")
#define VCREDIST_OPENMP_DIR GetEnv("VCREDIST_OPENMP_DIR")

Source: "{#VCREDIST_CRT_DIR}\*"; DestDir: "{#APP_BIN}"; Flags: recursesubdirs; Components: core;
Source: "{#VCREDIST_CXXAMP_DIR}\*"; DestDir: "{#APP_BIN}"; Flags: recursesubdirs; Components: core;
Source: "{#VCREDIST_OPENMP_DIR}\*"; DestDir: "{#APP_BIN}"; Flags: recursesubdirs; Components: core;

Source: "..\..\interfaces\ctcsound.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\*csnd6.pyd"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: python;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\*jcsound6.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
;; Source: "..\..\msvc\csound-vs\RelWithDebInfo\ableton_link_opcodes.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\ampmidid.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\arrayops.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\atsa.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\buchla.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\cellular.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\chua.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\cs.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\cs_date.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\csanalyze.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\csb64enc.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\csbeats.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\csdebugger.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\csnd6.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\csnd6.jar"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\csnd6.py"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: python;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\csound.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\csound64.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\csound64.lib"; DestDir: "{#APP_LIB}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\cvanal.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\dnoise.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\doppler.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\emugens.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\envext.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\exciter.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\extract.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\extractor.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\fareygen.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\fluidOpcodes.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\fractalnoise.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\framebuffer.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\ftsamplebank.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\getftargs.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\hdf5ops.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\het_export.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\het_import.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\hetro.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\image.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\ipmidi.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\linear_algebra.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\liveconv.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\lpanal.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\lpc_export.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\lpc_import.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\luaCsnd6.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\makecsd.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\mixer.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\mixer.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\osc.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\padsynth.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\platerev.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\pv_export.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\pv_import.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\pvanal.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\pvlook.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\pvsops.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\pmidi.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\py.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: python;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\quadbezier.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\rtpa.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\rtwinmm.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\scale.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\scansyn.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\scot.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\scsort.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\scugens.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\sdif2ad.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\select.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\serial.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\signalflowgraph.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\sndinfo.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\srconv.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\src_conv.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\stackops.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\stdutil.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\stkops.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\system_call.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\virtual.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\csound-vs\RelWithDebInfo\widgets.dll"; DestDir: "{#APP_PLUGINS64}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\deps\bin\*.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "..\..\msvc\deps\fluidsynthdeps\bin\*.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;

;; Source: "{#VcpkgInstalledBinDir}*.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion recursesubdirs;  Components: core;

;; Source: "..\..\msvc\csound-vs\RelWithDebInfo\libsndfile-1.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
;; Source: {#LibSndfileSourceDir}\include\*.*; DestDir: "{#APP_INCLUDE}\sndfile"; Flags: ignoreversion skipifsourcedoesntexist; Components: core;

Source: ../../include/*.h; DestDir: "{#APP_INCLUDE}\csound"; Flags: ignoreversion;  Components: core
Source: ../../include/*.hpp; DestDir: "{#APP_INCLUDE}\csound"; Flags: ignoreversion;  Components: core
Source: "../../msvc/csound-vs/include/float-version.h"; DestDir: "{#APP_INCLUDE}\csound"; Flags: ignoreversion;  Components: core
Source: ../../H/pffft.h; DestDir: "{#APP_INCLUDE}\csound"; Flags: ignoreversion;  Components: core
Source: ../../interfaces/*.h*; DestDir: "{#APP_INCLUDE}\csound"; Flags: ignoreversion;  Components: core
Source: ../../interfaces/csPerfThread.*; DestDir: "{#APP_INCLUDE}\csound"; Flags: ignoreversion;  Components: core
Source: ../../interfaces/*.py; DestDir: "{#APP_BIN}"; Flags: ignoreversion;  Components: core
Source: "../../examples\*.*"; DestDir: "{#APP_EXAMPLES}"; Excludes: "*.wav *.html"; Flags: ignoreversion recursesubdirs;  Components: core
Source: "../../samples\*.*"; DestDir: "{#APP_SAMPLES}"; Flags: ignoreversion recursesubdirs;  Components: core
Source: "{#LibStkSourceDir}rawwaves\*.*"; DestDir: "{#APP_SAMPLES}"; Flags: ignoreversion recursesubdirs;  Components: core
Source: {#ManualSourceDir}*.*; DestDir: "{#APP_MANUAL}"; Flags: ignoreversion recursesubdirs; Components: core
Source: {#CsoundQTDir}*.*; DestDir: "{app}\bin"; Flags: ignoreversion recursesubdirs; Components: csoundqt


[Icons]
Name: "{group}\{cm:ProgramOnTheWeb,Csound}"; Filename: "{#AppURL}";  Components: core;
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
Name: "{group}\Csound"; Filename: "cmd.exe"; Parameters: "/K csound.exe"; WorkingDir: "{#APP_BIN}"; Flags: dontcloseonexit;  Components: core
Name: "{group}\CsoundQt"; Filename: "{#APP_BIN}CsoundQt-d-cs6.exe"; WorkingDir: "{#APP_BIN}";  IconFilename:"{app}\{#CsoundQTIcon}" ;Components: csoundqt
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
