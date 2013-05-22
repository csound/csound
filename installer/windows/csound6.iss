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
; Cabbage, CsoundVST, CsoundQt, the Java API.
; I hope to change this soon.

#define MyAppName "Csound6"
#define MyAppVersion ""
#define MyAppMinVersion "0"
#define MyAppPublisher "Csound"
#define MyAppURL "http://sourceforge.net/projects/csound"
; If you are not Michael Gogins, change this to your MinGW dll directory.
#define MyMinGwBinDir "C:\mingw32-4.7.2\bin\"
; If you are not Michael Gogins, change this to your MinGW /usr/local/ directory.
#define MyMinGwUsrLocalDir "C:\mingw32-4.7.2\msys\1.0\local\"
; If you are not Michael Gogins, change this to your Csound build directory.
#define MySourceDir "C:\Users\Michael\csound-csound6-git\"
; If you are not Michael Gogins, change this to your Csound reference manual build directory.
#define MyManualSourceDir "C:\Users\Michael\csound-manual6-git\"
; If you are not Michael Gogins, change this to your Csound tutorial directory.
#define MyCsoundTutorialSourceDir "D:\Dropbox\tutorial\"
; If you are not Michael Gogins, change this to your CsoundAC tutorial directory.
#define MyCsoundAcTutorialSourceDir "D:\Dropbox\tutorial\"
; If you are not Michael Gogins, change this to your libsndfile install directory.
#define MyLibSndfileSourceDir "C:\mingw32-4.7.2\msys\1.0\local\lib\libsndfile\"
; If you are not Michael Gogins, change this to your PortAudio lib directory.
#define MyPortAudioSourceDir "C:\mingw32-4.7.2\msys\1.0\local\src\portaudio\"
; If you are not Michael Gogins, change this to your PortMidi build directory.
#define MyPortMidiSourceDir "C:\mingw32-4.7.2\msys\1.0\local\src\portmidi\"
; If you are not Michael Gogins, change this to your LuaJIT bin directory.
#define MyLuaJitSourceDir "C:\mingw32-4.7.2\msys\1.0\local\src\luajit-2.0\src\"
; If you are not Michael Gogins, change this to your FLTK dll directory.
#define MyFltkSourceDir "C:\mingw32-4.7.2\msys\1.0\local\src\fltk-1.3.2\src\"
; If you are not Michael Gogins, change this to your libmusicxml dll directory.
#define MyLibMusicXmlSourceDir "C:\mingw32-4.7.2\msys\1.0\local\src\libmusicxml-3.00-src\cmake\"
; If you are not Michael Gogins, change this to your FluidSynth dll directory.
#define MyFluidSynthSourceDir "C:\mingw32-4.7.2\msys\1.0\local\src\fluidsynth\src\.libs\"
; If you are not Michael Gogins, change this to your OSC library dll directory.
#define MyLibLoSourceDir "C:\mingw32-4.7.2\msys\1.0\local\src\liblo-0.26\"
; If you are not Michael Gogins, change this to your STK dll directory.
#define MyLibStkSourceDir "C:\mingw32-4.7.2\msys\1.0\local\src\stk-4.4.4\"
 
[Components]
Name: "core"; Description: "Core Csound"; Types: full custom; Flags: fixed
Name: "python"; Description: "Python features (requires Python 2.7)"; Types: full; 

[Dirs]
; ALL programs and shared libraries (including opcodes and other modules) go here.
Name: "{app}\bin"
#define APP_BIN "{app}\bin\"
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
LicenseFile=readme-csound6.txt
;InfoBeforeFile=readme-csound6.txt
OutputDir=installer\windows
OutputBaseFilename=Setup_{#MyAppName}_{#MyAppMinVersion}
Compression=lzma
SolidCompression=yes
SourceDir={#MySourceDir}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; NOTE: Don't use "Flags: ignoreversion" on any shared system files
Source: "{#MyMinGwBinDir}*.dll"; DestDir: "{#APP_BIN}"; Components: core;
; No idea why this other name is needed.
Source: "{#MyMinGwBinDir}libiconv-2.dll"; DestDir: "{#APP_BIN}"; DestName: "iconv.dll"; Components: core;
Source: "{#MyMinGwUsrLocalDir}bin/*.dll"; DestDir: "{#APP_BIN}"; Components: core;
Source: "*.exe"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: "*.dll*"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Excludes: "py.dll"; Components: core;
Source: "py.dll"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: python;
Source: "*.pyd"; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: python;
Source: "*.py";  DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: python;

Source: {#MyLibSndfileSourceDir}\bin\*.*; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core;
Source: {#MyLibSndfileSourceDir}\include\*.*; DestDir: "{#APP_INCLUDE}\sndfile"; Flags: ignoreversion; Components: core;

Source: {#MyPortAudioSourceDir}*.dll; DestDir: "{#APP_BIN}"; Components: core 
Source: {#MyPortAudioSourceDir}pa_devs.exe; DestDir: "{#APP_BIN}"; Components: core  
Source: {#MyPortAudioSourceDir}pa_minlat.exe; DestDir: "{#APP_BIN}"; Components: core  

Source: {#MyPortMidiSourceDir}*.dll; DestDir: "{#APP_BIN}"; Flags: ignoreversion; Components: core 

Source: {#MyLuaJitSourceDir}luajit.exe; DestDir: "{#APP_BIN}"; Flags: ignoreversion;  Components: core 

Source: {#MyLuaJitSourceDir}lua51.dll; DestDir: "{#APP_BIN}"; Flags: ignoreversion;  Components: core 
; No idea why this other name is needed.
Source: {#MyLuaJitSourceDir}lua51.dll; DestDir: "{#APP_BIN}"; DestName: "lua5.1.dll"; Flags: ignoreversion;  Components: core 
Source: {#MyLuaJitSourceDir}lua*.h; DestDir: "{#APP_INCLUDE}\luajit"; Flags: ignoreversion;  Components: core 

Source: {#MyFltkSourceDir}*.dll; DestDir: "{#APP_BIN}"; Flags: ignoreversion;  Components: core 

Source: {#MyLibMusicXmlSourceDir}*.dll; DestDir: "{#APP_BIN}"; Flags: ignoreversion;  Components: core 

Source: {#MyFluidSynthSourceDir}*.dll; DestDir: "{#APP_BIN}"; Flags: ignoreversion;  Components: core 

Source: {#MyLibLoSourceDir}*.dll; DestDir: "{#APP_BIN}"; Flags: ignoreversion;  Components: core 

Source: include/*.h*; DestDir: "{#APP_INCLUDE}\csound"; Flags: ignoreversion;  Components: core 
Source: include/interfaces/*.h*; DestDir: "{#APP_INCLUDE}\csound"; Flags: ignoreversion;  Components: core 
Source: frontends/CsoundAC/*.hpp; DestDir: "{#APP_INCLUDE}\csoundac"; Flags: ignoreversion;  Components: core 

Source: {#MyManualSourceDir}html\*.*; DestDir: "{#APP_MANUAL}"; Flags: ignoreversion recursesubdirs;  Components: core 

Source: "doc\csound\html\*.*"; DestDir: "{#APP_APIREF}/csound"; Flags: ignoreversion recursesubdirs;  Components: core 
Source: "doc\csoundac\html\*.*"; DestDir: "{#APP_APIREF}/csoundac"; Flags: ignoreversion recursesubdirs;  Components: core 

Source: "examples\*.*"; DestDir: "{#APP_EXAMPLES}"; Excludes: "*.wav"; Flags: ignoreversion recursesubdirs;  Components: core 

Source: "samples\*.*"; DestDir: "{#APP_SAMPLES}"; Flags: ignoreversion recursesubdirs;  Components: core 
Source: "{#MyLibStkSourceDir}rawwaves\*.*"; DestDir: "{#APP_SAMPLES}"; Flags: ignoreversion recursesubdirs;  Components: core 

Source: {#MyCsoundTutorialSourceDir}tutorial.pdf; DestDir: "{#APP_TUTORIAL}"; Flags: ignoreversion recursesubdirs;  Components: core 
Source: {#MyCsoundAcTutorialSourceDir}Csound_Algorithmic_Composition_Tutorial.pdf; DestDir: "{#APP_TUTORIAL}"; Flags: ignoreversion recursesubdirs;  Components: core 
Source: {#MyCsoundAcTutorialSourceDir}code\*.*; DestDir: "{#APP_TUTORIAL}code\"; Excludes: "*.wav"; Flags: ignoreversion recursesubdirs;  Components: core 

[Icons]
Name: "{group}\{cm:ProgramOnTheWeb,{#MyAppName}}"; Filename: "{#MyAppURL}";  Components: core;  
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{group}\Csound"; Filename: "cmd.exe"; Parameters: "/K csound.exe"; WorkingDir: "{#APP_BIN}"; Flags: dontcloseonexit;  Components: core  
Name: "{group}\WinSound"; Filename: "{#APP_BIN}winsound.exe"; WorkingDir: "{#APP_BIN}";  Components: core 
Name: "{group}\LuaJIT"; Filename: "{#APP_BIN}luajit.exe"; WorkingDir: "{#APP_BIN}";  Components: core 
Name: "{group}\Audio device information"; Filename: "cmd"; Parameters: "/K pa_devs.exe"; WorkingDir: "{#APP_BIN}"; Flags: dontcloseonexit;  Components: core 
Name: "{group}\Audio device latency"; Filename: "cmd"; Parameters: "/K pa_minlat.exe"; WorkingDir: "{#APP_BIN}"; Flags: dontcloseonexit;  Components: core 
Name: "{group}\Csound Reference Manual"; Filename: "{#APP_MANUAL}indexframes.html";  Components: core  
Name: "{group}\Csound API Reference Manual"; Filename: "{#APP_APIREF}csound\index.html";  Components: core  
Name: "{group}\CsoundAC API Reference Manual"; Filename: "{#APP_APIREF}csoundac\index.html";  Components: core  
Name: "{group}\Csound Tutorial"; Filename: "{#APP_TUTORIAL}tutorial.pdf";  Components: core  
Name: "{group}\CsoundAC Tutorial"; Filename: "{#APP_TUTORIAL}Csound_Algorithmic_Composition_Tutorial.pdf";  Components: core  

[Registry]
Root: HKCU; Subkey: "Environment"; ValueType:string; ValueName:"RAWWAVE_PATH"; ValueData:"{#APP_SAMPLES}"; Flags: preservestringtype uninsdeletekey;  Components: core 
Root: HKCU; Subkey: "Environment"; ValueType:string; ValueName:"OPCODE6DIR64"; ValueData:"{#APP_BIN}"; Flags: preservestringtype uninsdeletekey;  Components: core 
Root: HKCU; Subkey: "Environment"; ValueType:string; ValueName:"PYTHONPATH"; ValueData:"{#APP_BIN}"; Flags: preservestringtype uninsdeletekey;  Components: python 

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
	Result[0] := ExpandConstant('{app}/bin');
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

