; -- csound~.iss --
; Inno Setup build file for csound~ for Windows
; Author: Steven Yi<stevenyi@gmail.com>

; This should be supplied by the calling build-installer-windows.sh script
;#define CS_TILDE_VERSION "xxx"

[Setup]
AppName=csound~
AppVersion={#CS_TILDE_VERSION}
DefaultDirName={userdocs}\Max\Packages\csound~_v{#CS_TILDE_VERSION}
DefaultGroupName=csound~
UninstallDisplayIcon={app}\csound~.mxe
Compression=lzma2
SolidCompression=yes
OutputDir=userdocs:Inno Setup Examples Output
LicenseFile=..\..\..\LICENSE.TXT

[Files]
Source: "csound~.mxe"; DestDir: "{app}"
Source: "..\..\..\help\csound~\*"; DestDir: "{app}\help\csound~"
Source: "..\..\..\examples\*"; DestDir: "{app}\examples"

;Source: "MyProg.chm"; DestDir: "{app}"
;Source: "Readme.txt"; DestDir: "{app}"; Flags: isreadme

[Icons]
Name: "{group}\csound~"; Filename: "{app}\csound~.mxe"
