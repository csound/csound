;NSIS Modern Csound5 Install Script

!define PRODUCT "Csound"
!define VERSION "5beta"
!define PROGRAM "csound"

!include "MUI.nsh"

;--------------------------------
;General

  ;Name and file
  Name "${PRODUCT}"
  OutFile "${PROGRAM}-${VERSION}.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\${PRODUCT}"
  
  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\${PRODUCT}" ""

;--------------------------------
;Variables

  Var MUI_TEMP
  Var STARTMENU_FOLDER

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "..\..\readme.txt"
  !insertmacro MUI_PAGE_DIRECTORY
  
  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU" 
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${PRODUCT}" 
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
  
  !insertmacro MUI_PAGE_STARTMENU Application $STARTMENU_FOLDER
  
  !insertmacro MUI_PAGE_INSTFILES

  !define MUI_FINISHPAGE_SHOWREADME readme.txt
  !define MUI_FINISHPAGE_SHOWREADME_CHECKED
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

Section "${PRODUCT}" SecCopyUI

  SetOutPath "$INSTDIR"
  
  File ..\..\*.exe
  File ..\..\*.dll
  File ..\..\*.xmg
  File ..\..\*.txt
  File ..\..\ChangeLog
  File ..\..\loris.py
  File ..\..\CsoundVST.py
  file ..\..\CsoundVST.jar
  SetOutPath $INSTDIR\lib
  File ..\..\libcsound.a
  File ..\..\libCsoundVST.a
  SetOutPath $INSTDIR\include
  File ..\..\H\*.h
  File ..\..\frontends\CsoundVST\*.h
  File ..\..\frontends\CsoundVST\*.hpp
  SetOutPath $INSTDIR\examples
  File ..\..\examples\*.csd
  File ..\..\examples\*.py
  SetOutPath $INSTDIR\samples
  File ..\..\samples\*

  ;Store installation folder
  WriteRegStr HKCU "Software\${PRODUCT}" "" $INSTDIR
  
  ; back up old value of .csd
  ReadRegStr $1 HKCR ".csd" ""
  StrCmp $1 "" noBackup
    StrCmp $1 "${PRODUCT}File" noBackup
    WriteRegStr HKCR ".csd" "backup_val" $1
  noBackup:
  WriteRegStr HKCR ".csd" "" "${PRODUCT}File"
  ReadRegStr $0 HKCR "${PRODUCT}File" ""
  StrCmp $0 "" 0 skipAssoc
	WriteRegStr HKCR "${PRODUCT}File" "" "CSound Unified File"
	WriteRegStr HKCR "${PRODUCT}File\shell" "" "open"
	WriteRegStr HKCR "${PRODUCT}File\DefaultIcon" "" $INSTDIR\${PROGRAM}.exe,0
  skipAssoc:
  WriteRegStr HKCR "${PRODUCT}File\shell\open\command" "" '$INSTDIR\${PROGRAM}.exe "%1"'

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    
    ;Create shortcuts
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\${PRODUCT}.lnk" "$INSTDIR\${PROGRAM}.exe"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\CsoundVST.lnk" "$INSTDIR\CsoundVST.exe"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\uninstall.exe"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\License.lnk" "$INSTDIR\license.txt"
  
  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecCopyUI ${LANG_ENGLISH} "Copy ${PRODUCT} to the application folder."

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCopyUI} $(DESC_SecCopyUI)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  Delete "$INSTDIR\*.exe"
  Delete "$INSTDIR\*.dll"
  Delete "$INSTDIR\*.xmg"
  Delete "$INSTDIR\csound-*.pdf"
  Delete "$INSTDIR\*.htm*"
  Delete "$INSTDIR\*.txt"
  Delete "$INSTDIR\ChangeLog"
  Delete "$INSTDIR\loris.py"
  Delete "$INSTDIR\CsoundVST.py"
  Delete "$INSTDIR\*.class"

  Delete "$INSTDIR\lib\libcsound.a"
  Delete "$INSTDIR\lib\libCsoundVST.a"
  RMDir "$INSTDIR\lib"

  Delete "$INSTDIR\include\csound.h"
  Delete "$INSTDIR\include\*.hpp"
  RMDir "$INSTDIR\include"

  Delete "$INSTDIR\examples\*.csd"
  Delete "$INSTDIR\examples\*.py"
  RMDir "$INSTDIR\examples"

  Delete "$INSTDIR\samples\*"
  RMDir "$INSTDIR\samples"

  RMDir "$INSTDIR"
  
  !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
    
  Delete "$SMPROGRAMS\$MUI_TEMP\${PRODUCT}.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Overview.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\License.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"
  
  ;Delete empty start menu parent diretories
  StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"
 
  startMenuDeleteLoop:
    RMDir $MUI_TEMP
    GetFullPathName $MUI_TEMP "$MUI_TEMP\.."
    
    IfErrors startMenuDeleteLoopDone
  
    StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
  startMenuDeleteLoopDone:

  DeleteRegKey /ifempty HKCU "Software\${PRODUCT}"

SectionEnd
