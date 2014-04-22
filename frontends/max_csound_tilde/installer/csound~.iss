; -- csound~.iss --
; Inno Setup build file for csound~ for Windows
; Author: Steven Yi<stevenyi@gmail.com>

[Setup]
AppName=csound~
AppVersion=1.1.2
DefaultDirName={userdocs}\Max\Packages\csound~_v1.1.2
DefaultGroupName=csound~
UninstallDisplayIcon={app}\csound~.mxe.dll
Compression=lzma2
SolidCompression=yes
OutputDir=userdocs:Inno Setup Examples Output

[Files]
Source: "csound~.mxe"; DestDir: "{app}"
;Source: "MyProg.chm"; DestDir: "{app}"
;Source: "Readme.txt"; DestDir: "{app}"; Flags: isreadme

[Icons]
Name: "{group}\csound~"; Filename: "{app}\csound~.mxe"
