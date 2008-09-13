######################################################################
# C S O U N D   5   N U L L S O F T   I N S T A L L E R   S C R I P T
# By Michael Gogins <gogins@pipeline.com>
#
# If this script is compiled with the /DFLOAT option,
# the installer will install the 'float samples' version of Csound;
# by default, the installer will install the 'double samples' version.
# If this script is compiled with the /DNONFREE option,
# the installer will install non-free software (stk.dll and CsoundVST);
# by default, the installer will omit all non-free software.
#######################################################################

!include MUI.nsh
!include WinMessages.nsh

#######################################################################
# DEFINITIONS
#######################################################################

!define PRODUCT "Csound"
!define PROGRAM "Csound5.09.0"
!echo "Building installer for: ${PROGRAM}"
!ifdef FLOAT
!ifdef NONFREE
!define VERSION "gnu-win32-vst-f"
!else
!define VERSION "gnu-win32-f"
!endif
!echo "Building installer for single-precision samples."
!define OPCODEDIR_ENV "OPCODEDIR"
!define OPCODEDIR_VAL "plugins"
!else
!ifdef NONFREE
!define VERSION "gnu-win32-vst-d"
!else
!define VERSION "gnu-win32-d"
!endif
!echo "Building installer for double-precision samples."
!define OPCODEDIR_ENV "OPCODEDIR64"
!define OPCODEDIR_VAL "plugins64"
!endif
!ifdef NONFREE
!echo "Building installer with VST software."
!else
!echo "Building installer without VST software."
!endif

!define ALL_USERS
!ifdef ALL_USERS
!define WriteEnvStr_RegKey 'HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"'
!else
!define WriteEnvStr_RegKey 'HKCU "Environment"'
!endif

!define PYTHON_VERSION 2.5

!define MUI_ABORTWARNING

#######################################################################
# GENERAL
#######################################################################

Name "${PRODUCT}"
OutFile "${PROGRAM}-${VERSION}.exe"
InstallDir "$PROGRAMFILES\${PRODUCT}"
# Get installation folder from registry if Csound is already installed.
InstallDirRegKey HKCU "Software\${PRODUCT}" ""

#######################################################################
# VARIABLES
#######################################################################

Var MUI_TEMP
Var STARTMENU_FOLDER
Var PYTHON_OUTPUT_PATH

#######################################################################
# FUNCTIONS
#######################################################################

# Check to see if the required version of Python is installed;
# if not, ask the user if he or she wants to install it;
# if so, try to install it.
# if Python is installed, enable the Python features of Csound.
# If Python is available, or not available, then
# a Python backup output path is not defined, or defined, respectively.

Function GetPython
	ClearErrors
	StrCpy $3 "Software\Python\PythonCore\${PYTHON_VERSION}\InstallPath"
	ReadRegStr $1 HKLM $3 ""
  	IfErrors tryToInstallPython enablePython
tryToInstallPython:
	MessageBox MB_OKCANCEL "You don't have Python ${PYTHON_VERSION} installed. Continue installing Csound without Python features (OK), or quit installing Csound so you can install Python first (Cancel)?" IDOK disablePython IDCANCEL installPython
installPython:
   	Abort 
enablePython:
	DetailPrint "Python ${PYTHON_VERSION} is available, enabling Python features..."
	StrCpy $PYTHON_OUTPUT_PATH ""
	Goto done
disablePython:
	DetailPrint "Python ${PYTHON_VERSION} is not available. Python modules will be installed in the 'python_backup' directory."
	StrCpy $PYTHON_OUTPUT_PATH "\python_backup"
done:
FunctionEnd

# WriteEnvStr - Write an environment variable
# Note: Win9x systems requires reboot
#
# Example:
#  Push "HOMEDIR"           # name
#  Push "C:\New Home Dir\"  # value
#  Call WriteEnvStr

Function WriteEnvStr
	 Exch $1 # $1 has environment variable value
	 Exch
	 Exch $0 # $0 has environment variable name
	 Push $2
	 Call IsNT
	 Pop $2
  	 StrCmp $2 1 WriteEnvStr_NT
    	 # Not on NT
    	 StrCpy $2 $WINDIR 2 # Copy drive of windows (c:)
    	 FileOpen $2 "$2\autoexec.bat" a
    	 FileSeek $2 0 END
    	 FileWrite $2 "$\r$\nSET $0=$1$\r$\n"
    	 FileClose $2
    	 SetRebootFlag true
	 Goto WriteEnvStr_done
WriteEnvStr_NT:
	WriteRegExpandStr ${WriteEnvStr_RegKey} $0 $1
	SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
WriteEnvStr_done:
	Pop $2
    	Pop $1
    	Pop $0
FunctionEnd

# un.DeleteEnvStr - Remove an environment variable
# Note: Win9x systems requires reboot
#
# Example:
#  Push "HOMEDIR"           # name
#  Call un.DeleteEnvStr

Function un.DeleteEnvStr
  	Exch $0 # $0 now has the name of the variable
  	Push $1
  	Push $2
  	Push $3
  	Push $4
  	Push $5
	Call un.IsNT
  	Pop $1
  	StrCmp $1 1 DeleteEnvStr_NT
    	# Not on NT
    	StrCpy $1 $WINDIR 2
    	FileOpen $1 "$1\autoexec.bat" r
    	GetTempFileName $4
    	FileOpen $2 $4 w
    	StrCpy $0 "SET $0="
    	SetRebootFlag true
DeleteEnvStr_dosLoop:
	FileRead $1 $3
      	StrLen $5 $0
      	StrCpy $5 $3 $5
      	StrCmp $5 $0 DeleteEnvStr_dosLoop
      	StrCmp $5 "" DeleteEnvStr_dosLoopEnd
      	FileWrite $2 $3
      	Goto DeleteEnvStr_dosLoop
DeleteEnvStr_dosLoopEnd:
	FileClose $2
      	FileClose $1
      	StrCpy $1 $WINDIR 2
      	Delete "$1\autoexec.bat"
      	CopyFiles /SILENT $4 "$1\autoexec.bat"
      	Delete $4
      	Goto DeleteEnvStr_done
DeleteEnvStr_NT:
	DeleteRegValue ${WriteEnvStr_RegKey} $0
    	SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
DeleteEnvStr_done:
	Pop $5
    	Pop $4
    	Pop $3
    	Pop $2
    	Pop $1
    	Pop $0
FunctionEnd

;----------------------------------------
; based upon a script of "Written by KiCHiK 2003-01-18 05:57:02"
;----------------------------------------
!verbose 3
!include "WinMessages.NSH"
!verbose 4
;====================================================
; get_NT_environment 
;     Returns: the selected environment
;     Output : head of the stack
;====================================================
!macro select_NT_profile UN
Function ${UN}select_NT_profile
   MessageBox MB_YESNO|MB_ICONQUESTION "Change the environment for all users?$\r$\nSaying no here will change the envrironment for the current user only.$\r$\n(Administrator permissions required for all users)" \
      IDNO environment_single
      DetailPrint "Selected environment for all users"
      Push "all"
      Return
   environment_single:
      DetailPrint "Selected environment for current user only."
      Push "current"
      Return
FunctionEnd
!macroend
!insertmacro select_NT_profile ""
!insertmacro select_NT_profile "un."
;----------------------------------------------------
!define NT_current_env 'HKCU "Environment"'
!define NT_all_env     'HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"'
;====================================================
; IsNT - Returns 1 if the current system is NT, 0
;        otherwise.
;     Output: head of the stack
;====================================================
!macro IsNT UN
Function ${UN}IsNT
  Push $0
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
  StrCmp $0 "" 0 IsNT_yes
  ; we are not NT.
  Pop $0
  Push 0
  Return
 
  IsNT_yes:
    ; NT!!!
    Pop $0
    Push 1
FunctionEnd
!macroend
!insertmacro IsNT ""
!insertmacro IsNT "un."
;====================================================
; AddToPath - Adds the given dir to the search path.
;        Input - head of the stack
;        Note - Win9x systems requires reboot
;====================================================
Function AddToPath
   Exch $0
   Push $1
   Push $2
  
   Call IsNT
   Pop $1
   StrCmp $1 1 AddToPath_NT
      ; Not on NT
      StrCpy $1 $WINDIR 2
      FileOpen $1 "$1\autoexec.bat" a
      FileSeek $1 0 END
      GetFullPathName /SHORT $0 $0
      FileWrite $1 "$\r$\nSET PATH=%PATH%;$0$\r$\n"
      FileClose $1
      Goto AddToPath_done
 
   AddToPath_NT:
      Push $4
      Call select_NT_profile
      Pop  $4
 
      AddToPath_NT_selection_done:
      StrCmp $4 "current" read_path_NT_current
         ReadRegStr $1 ${NT_all_env} "PATH"
         Goto read_path_NT_resume
      read_path_NT_current:
         ReadRegStr $1 ${NT_current_env} "PATH"
      read_path_NT_resume:
      StrCpy $2 $0
      StrCmp $1 "" AddToPath_NTdoIt
         StrCpy $2 "$1;$0"
      AddToPath_NTdoIt:
         StrCmp $4 "current" write_path_NT_current
            ClearErrors
            WriteRegExpandStr ${NT_all_env} "PATH" $2
            IfErrors 0 write_path_NT_resume
            MessageBox MB_YESNO|MB_ICONQUESTION "The path could not be set for all users$\r$\nShould I try for the current user?" \
               IDNO write_path_NT_failed
            ; change selection
            StrCpy $4 "current"
            Goto AddToPath_NT_selection_done
         write_path_NT_current:
            ClearErrors
            WriteRegExpandStr ${NT_current_env} "PATH" $2
            IfErrors 0 write_path_NT_resume
            MessageBox MB_OK|MB_ICONINFORMATION "The path could not be set for the current user."
            Goto write_path_NT_failed
         write_path_NT_resume:
         SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
         DetailPrint "added path for user ($4), $0"
         write_path_NT_failed:
      
      Pop $4
   AddToPath_done:
   Pop $2
   Pop $1
   Pop $0
FunctionEnd
 
;====================================================
; RemoveFromPath - Remove a given dir from the path
;     Input: head of the stack
;====================================================
Function un.RemoveFromPath
   Exch $0
   Push $1
   Push $2
   Push $3
   Push $4
   
   Call un.IsNT
   Pop $1
   StrCmp $1 1 unRemoveFromPath_NT
      ; Not on NT
      StrCpy $1 $WINDIR 2
      FileOpen $1 "$1\autoexec.bat" r
      GetTempFileName $4
      FileOpen $2 $4 w
      GetFullPathName /SHORT $0 $0
      StrCpy $0 "SET PATH=%PATH%;$0"
      SetRebootFlag true
      Goto unRemoveFromPath_dosLoop
     
      unRemoveFromPath_dosLoop:
         FileRead $1 $3
         StrCmp $3 "$0$\r$\n" unRemoveFromPath_dosLoop
         StrCmp $3 "$0$\n" unRemoveFromPath_dosLoop
         StrCmp $3 "$0" unRemoveFromPath_dosLoop
         StrCmp $3 "" unRemoveFromPath_dosLoopEnd
         FileWrite $2 $3
         Goto unRemoveFromPath_dosLoop
 
      unRemoveFromPath_dosLoopEnd:
         FileClose $2
         FileClose $1
         StrCpy $1 $WINDIR 2
         Delete "$1\autoexec.bat"
         CopyFiles /SILENT $4 "$1\autoexec.bat"
         Delete $4
         Goto unRemoveFromPath_done
 
   unRemoveFromPath_NT:
      StrLen $2 $0
      Call un.select_NT_profile
      Pop  $4
 
      StrCmp $4 "current" un_read_path_NT_current
         ReadRegStr $1 ${NT_all_env} "PATH"
         Goto un_read_path_NT_resume
      un_read_path_NT_current:
         ReadRegStr $1 ${NT_current_env} "PATH"
      un_read_path_NT_resume:
 
      Push $1
      Push $0
      Call un.StrStr ; Find $0 in $1
      Pop $0 ; pos of our dir
      IntCmp $0 -1 unRemoveFromPath_done
         ; else, it is in path
         StrCpy $3 $1 $0 ; $3 now has the part of the path before our dir
         IntOp $2 $2 + $0 ; $2 now contains the pos after our dir in the path (';')
         IntOp $2 $2 + 1 ; $2 now containts the pos after our dir and the semicolon.
         StrLen $0 $1
         StrCpy $1 $1 $0 $2
         StrCpy $3 "$3$1"
 
         StrCmp $4 "current" un_write_path_NT_current
            WriteRegExpandStr ${NT_all_env} "PATH" $3
            Goto un_write_path_NT_resume
         un_write_path_NT_current:
            WriteRegExpandStr ${NT_current_env} "PATH" $3
         un_write_path_NT_resume:
         SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
   unRemoveFromPath_done:
   Pop $4
   Pop $3
   Pop $2
   Pop $1
   Pop $0
FunctionEnd
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Uninstall sutff
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 
 
;====================================================
; StrStr - Finds a given string in another given string.
;               Returns -1 if not found and the pos if found.
;          Input: head of the stack - string to find
;                      second in the stack - string to find in
;          Output: head of the stack
;====================================================
Function un.StrStr
  Push $0
  Exch
  Pop $0 ; $0 now have the string to find
  Push $1
  Exch 2
  Pop $1 ; $1 now have the string to find in
  Exch
  Push $2
  Push $3
  Push $4
  Push $5
 
  StrCpy $2 -1
  StrLen $3 $0
  StrLen $4 $1
  IntOp $4 $4 - $3
 
  unStrStr_loop:
    IntOp $2 $2 + 1
    IntCmp $2 $4 0 0 unStrStrReturn_notFound
    StrCpy $5 $1 $3 $2
    StrCmp $5 $0 unStrStr_done unStrStr_loop
 
  unStrStrReturn_notFound:
    StrCpy $2 -1
 
  unStrStr_done:
    Pop $5
    Pop $4
    Pop $3
    Exch $2
    Exch 2
    Pop $0
    Pop $1
FunctionEnd
;====================================================Retrieved from "http://nsis.sourceforge.net/Path_manipulation_with_all_users/current_user_selection_in_run-time"


#######################################################################
# INSTALLER PAGES
#######################################################################
  
!insertmacro MUI_PAGE_WELCOME

!ifdef NONFREE
!insertmacro MUI_PAGE_LICENSE ..\..\readme-csound5-complete.txt
!else
!insertmacro MUI_PAGE_LICENSE ..\..\readme-csound5.txt
!endif

!insertmacro MUI_PAGE_DIRECTORY

# Start Menu Folder Page Configuration
  
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU" 
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${PRODUCT}" 
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
  
!insertmacro MUI_PAGE_STARTMENU Application $STARTMENU_FOLDER
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
 
#######################################################################
# LANGUAGES AND DESCRIPTIONS
#######################################################################

!insertmacro MUI_LANGUAGE "English"

LangString DESC_SecCopyUI ${LANG_ENGLISH} "Copy ${PRODUCT} to the application folder."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${SecCopyUI} $(DESC_SecCopyUI)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

#######################################################################
# INSTALLER SECTIONS
#######################################################################

Section "${PRODUCT}" SecCopyUI
	# If Python is not available, ask the user if they want to install it.
	Call GetPython
	# First we do output paths for non-Python stuff.
	SetOutPath $INSTDIR\bin\Microsoft.VC90.CRT
	File U:\msvc2008\VC\redist\x86\Microsoft.VC90.CRT\*

	# Outputs to root installation directory.

  	SetOutPath $INSTDIR
	# Exclude junk soundfiles.
	File /nonfatal /x test ..\..\test
	# Exclude all Python stuff. We will install it later, perhaps.
	File /nonfatal /x  ..\..\*.pyd  ..\..\csnd.dll  ..\..\csnd.py  ..\..\CsoundAC.dll  ..\..\CsoundAC.py  ..\..\py.dll 
!ifdef NONFREE
	File ..\..\readme-csound5-complete.txt
!else
	File ..\..\readme-csound5.txt
!endif
	File ..\..\etc\.csoundrc
  	File ..\..\INSTALL

	# Outputs to bin directory.

  	SetOutPath $INSTDIR\bin
!ifdef FLOAT
	File ..\..\csound32.dll.5.1
!else
	File ..\..\csound64.dll.5.1
!endif
	File C:\windows\system32\MSVCRT.DLL
	File ..\..\csnd.dll
  	File ..\..\_jcsound.dll
!ifdef NONFREE
	File ..\..\CsoundVST.dll
	File ..\..\*.exe
!else
	File /x CsoundVSTShell.exe ..\..\*.exe
!endif
	File ..\..\tclcsound.dll
  	File ..\..\csoundapi~.dll

        # Third party libraries:

	# libsndfile
	File U:\Mega-Nerd\libsndfile\libsndfile-1.dll
	# FLTK
	File U:\fltk-mingw\src\mgwfltknox-1.3.dll
	File U:\fltk-mingw\src\mgwfltknox_images-1.3.dll
	# PortAudio
	File U:\portaudio\build\msvc\Win32\Release\portaudio_x86.dll
	# PortMIDI
	File U:\portmidi\pm_win\Release\pm_dll.dll
	# Fluidsynth
	File U:\fluidsynth.patched\winbuild\fluidsynth.dll
	# Image opcodes
	File U:\zlib-1.2.3.win32\bin\zlib1.dll
	File U:\libpng-1.2.24\.libs\libpng-3.dll
	# Lua
	File U:\Lua5.1\lua51.dll
	File U:\Lua5.1\luajit.exe
	# OSC
	File U:\liblo\lo.dll
	File U:\pthreads\Pre-built.2\lib\pthreadGC2.dll

!ifdef FLOAT
  	File ..\..\csound32.def
!else
  	File ..\..\csound64.def
!endif
  	File ..\..\_csnd.def
  	File ..\..\_jcsound.def
#ifdef NONFREE
	File ..\..\frontends\CsoundVST\_CsoundVST.def
#endif
	SetOutPath $INSTDIR\pluginSDK
  	File ..\..\pluginSDK\SConstruct
  	File ..\..\pluginSDK\examplePlugin.c
  	File ..\..\pluginSDK\custom.py
  	SetOutPath $INSTDIR\doc
  	File ..\..\*.txt
  	File ..\..\doc\latex\refman.pdf
  	File ..\..\ChangeLog
  	File ..\..\COPYING
  	File ..\..\LICENSE.PortMidi
  	File ..\..\LICENSE.FLTK
  	File ..\..\LICENSE.PortAudio
  	SetOutPath $INSTDIR\doc\manual
  	File /r ..\..\..\manual\html\*
  	SetOutPath $INSTDIR\tutorial
  	File ..\..\..\tutorial\tutorial.pdf
  	File ..\..\..\tutorial\*.csd
  	File ..\..\..\tutorial\*.py
  	File ..\..\..\tutorial\tutorial3.cpr
  	SetOutPath $INSTDIR\examples
  	File /x *.wav /x *.orc /x *.sco ..\..\examples\*.*
  	SetOutPath $INSTDIR\examples\csoundapi_tilde
  	File /x *.wav /x *.orc /x *.sco ..\..\examples\csoundapi_tilde\*.*
  	SetOutPath $INSTDIR\examples\java
  	File /x *.wav /x *.orc /x *.sco ..\..\examples\java\*.*
  	SetOutPath $INSTDIR\examples\tclcsound
  	File /x *.wav /x *.orc /x *.sco ..\..\examples\tclcsound\*.*
  	SetOutPath $INSTDIR\examples\gab
  	File /x *.wav /x *.orc /x *.sco ..\..\Opcodes\gab\examples\*.*
  	SetOutPath $INSTDIR\examples\py
  	File /x *.wav  ..\..\Opcodes\py\examples\*.*
  	SetOutPath $INSTDIR\include
  	File ..\..\H\*.h
  	File ..\..\H\*.hpp
  	File ..\..\interfaces\*.hpp
!ifdef NONFREE
        File ..\..\frontends\CsoundVST\*.h
       	File ..\..\frontends\CsoundVST\*.hpp
!endif
	File ..\..\frontends\CsoundAC\*.h
	File ..\..\frontends\CsoundAC\*.hpp
  	SetOutPath $INSTDIR\interfaces\java
  	File ..\..\csnd.jar
  	SetOutPath $INSTDIR\interfaces\lisp
  	File ..\..\interfaces\*.lisp
  	SetOutPath $INSTDIR\samples
  	File /r ..\..\samples\*
	File /r ..\..\Opcodes\stk\rawwaves\*.raw
	File ..\..\CompositionBase.py

	# Then we do opcodes, which are a special case with respect to Python and non-free software.

	SetOutPath $INSTDIR\${OPCODEDIR_VAL}
	File ..\..\rtpa.dll
	File ..\..\rtpa.dll
!ifdef NONFREE
	DetailPrint "Free and non-free software."
	File /x csound*.dll* \
	/x *.pyd \       
	/x libsndfile-1.dll \
	/x portaudio*.dll* \
	/x tclcsound.dll \
	/x csoundapi~.dll \
	/x py.dll \
	/x pm_midi.dll \
	..\..\*.dll \
	..\..\frontends\csladspa\csladspa.dll
!else
	DetailPrint "Only free software."
	File /x csound*.dll* \
	/x vst4cs.dll \
	/x *.pyd \       
	/x libsndfile-1.dll \
	/x portaudio.dll* \
	/x tclcsound.dll \
	/x csoundapi~.dll \
	/x py.dll \
	/x pm_midi.dll \
	..\..\*.dll \
	..\..\frontends\csladspa\csladspa.dll
!endif
	DetailPrint "Python executables."
	
  	SetOutPath $INSTDIR$PYTHON_OUTPUT_PATH\bin
	File ..\..\_CsoundAC.pyd  ..\..\CsoundAC.py  ..\..\_csnd.pyd  ..\..\csnd.py

	DetailPrint "Python opcodes."

	SetOutPath $INSTDIR$PYTHON_OUTPUT_PATH\${OPCODEDIR_VAL}
	File ..\..\py.dll \

	# Store the installation folder.
	WriteRegStr HKCU "Software\${PRODUCT}" "" $INSTDIR
	# Back up any old value of .csd.
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
	WriteRegStr HKCR "${PRODUCT}File\DefaultIcon" "" $INSTDIR\bin\${PROGRAM}.exe,0
skipAssoc:
	WriteRegStr HKCR "${PRODUCT}File\shell\open\command" "" '$INSTDIR\bin${PROGRAM}.exe "%1"'
	Push $INSTDIR\bin
	Call AddToPath
     	Push "CSOUNDRC" 
	Push $INSTDIR\.csoundrc
	Call WriteEnvStr
	Push ${OPCODEDIR_ENV}
	Push $INSTDIR\${OPCODEDIR_VAL}
	Call WriteEnvStr
	Push "RAWWAVE_PATH" 
	Push "$INSTDIR\samples"
	Call WriteEnvStr
	ReadEnvStr $1 "PYTHONPATH"
	Push "PYTHONPATH"
	Push "$1;$INSTDIR\bin"
	Call WriteEnvStr
	Push "SFOUTYP"
	Push "WAV"
	Call WriteEnvStr
	WriteUninstaller "$INSTDIR\Uninstall.exe"
	!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
	# Create shortcuts. The format of these lines is:
	# link.lnk target.file [parameters [icon.file [icon_index_number [start_options [keyboard_shortcut [description]]]]]]
	CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\${PRODUCT}.lnk" "$INSTDIR\bin\csound.exe" "" "" "" "" "" "Command-line Csound"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\csound5gui.lnk" "$INSTDIR\bin\csound5gui.exe" "" "" "" "" "" " Varga Csound GUI"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\winsound.lnk" "$INSTDIR\bin\winsound.exe" "" "" "" "" "" "ffitch Csound GUI"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\cseditor.lnk" "$INSTDIR\bin\cseditor.exe" "" "" "" "" "" "Csound editor"
!ifdef NONFREE
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\CsoundVST.lnk" "$INSTDIR\bin\CsoundVSTShell.exe" "" "" "" "" "" "CsoundVST GUI"
!endif
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\License.lnk" "$INSTDIR\doc\readme-csound5-complete.txt" "" "" "" "" "" "Csound README"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Manual.lnk" "$INSTDIR\doc\manual\indexframes.html" "" "" "" "" "" "Csound manual"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Tutorial.lnk" "$INSTDIR\tutorial\tutorial.pdf" "" "" "" "" "" "Csound tutorial"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\API Reference.lnk" "$INSTDIR\doc\refman.pdf" "" "" "" "" "" "API reference"
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "" "" "" "" "Uninstall Csound"
	!insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section "Uninstall"
  	RMDir /r $INSTDIR
  	!insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
  	Delete "$SMPROGRAMS\$MUI_TEMP\${PRODUCT}.lnk"
  	Delete "$SMPROGRAMS\$MUI_TEMP\csound5gui.lnk"
  	Delete "$SMPROGRAMS\$MUI_TEMP\winsound.lnk"
  	Delete "$SMPROGRAMS\$MUI_TEMP\cseditor.lnk"
!ifdef NONFREE
	Delete "$SMPROGRAMS\$MUI_TEMP\CsoundVST.lnk"
!endif
	Delete "$SMPROGRAMS\$MUI_TEMP\License.lnk"
  	Delete "$SMPROGRAMS\$MUI_TEMP\Manual.lnk"
  	Delete "$SMPROGRAMS\$MUI_TEMP\Tutorial.lnk"
  	Delete "$SMPROGRAMS\$MUI_TEMP\API Reference.lnk"
  	Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"
	# Delete empty start menu parent dircetories.
  	StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"
startMenuDeleteLoop:
	RMDir $MUI_TEMP
    	GetFullPathName $MUI_TEMP "$MUI_TEMP\.."
    	IfErrors startMenuDeleteLoopDone
    	StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
startMenuDeleteLoopDone:
	Push $INSTDIR
  	Call un.RemoveFromPath
  	Push "CSOUNDRC"
  	Call un.DeleteEnvStr 
	Push ${OPCODEDIR_ENV}
  	Call un.DeleteEnvStr 
!ifdef NONFREE
        Push "RAWWAVE_PATH"
       	Call un.DeleteEnvStr 
!endif
	Push "SFOUTYP"
  	Call un.DeleteEnvStr
  	DeleteRegKey /ifempty HKCU "Software\${PRODUCT}"
SectionEnd
