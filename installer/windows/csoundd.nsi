#######################################################################
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
!define PROGRAM "Csound5.06"
!echo "Building installer for: ${PROGRAM}"
!ifdef FLOAT
	!define VERSION "win32-f"
	!echo "Building installer for single-precision samples."
	!define OPCODEDIR_ENV "OPCODEDIR"
	!define OPCODEDIR_VAL "plugins"
!else
	!define VERSION "win32-d"
	!echo "Building installer for double-precision samples."
	!define OPCODEDIR_ENV "OPCODEDIR64"
	!define OPCODEDIR_VAL "plugins64"
!endif
!ifdef NONFREE
	!echo "Building installer with VST and STK software."
!else
	!echo "Building installer without VST and STK software."
!endif

!define ALL_USERS
!ifdef ALL_USERS
	!define WriteEnvStr_RegKey 'HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"'
!else
  !define WriteEnvStr_RegKey 'HKCU "Environment"'
!endif

!define PYTHON_VERSION 2.5
!define PYTHON_URL "http://www.python.org/ftp/python/2.5.1/python-2.5.1.msi"

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
	MessageBox MB_YESNO "Csound can be scripted with Python ${PYTHON_VERSION}. Install Python ${PYTHON_VERSION}?" IDYES installPython IDNO disablePython
installPython:
   	DetailPrint "Attempting to install Python..."
       	StrCpy $2 "$TEMP\python-2.5.1.msi"
        nsisdl::download /TIMEOUT=30000 ${PYTHON_URL} $2
        Pop $R0 #Get the return value
        StrCmp $R0 "success" +3
        MessageBox MB_OK "Download failed: $R0"
        Goto disablePython
        ExecWait $2
        Delete $2
enablePython:
	DetailPrint "Python ${PYTHON_VERSION} is available, enabling Python features..."
	StrCpy $PYTHON_OUTPUT_PATH ""
	Goto done
disablePython:
	DetailPrint "Python ${PYTHON_VERSION} is not available. Python modules will be installed in the 'python_backup' directory."
	StrCpy $PYTHON_OUTPUT_PATH "python_backup\"
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

# [un.]IsNT - Push 1 if running on NT, 0 if not
#
# Example:
#   Call IsNT
#   Pop $0
#   StrCmp $0 1 +3
#     MessageBox MB_OK "Not running on NT!"
#     Goto +2
#     MessageBox MB_OK "Running on NT!"

!macro IsNT UN
Function ${UN}IsNT
  Push $0
  ReadRegStr $0 HKLM \
    "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
  StrCmp $0 "" 0 IsNT_yes
  	# we are not NT.
  Pop $0
  Push 0
  Return
IsNT_yes:
    	# NT!!!
    Pop $0
    Push 1
FunctionEnd
!macroend
!insertmacro IsNT ""
!insertmacro IsNT "un."

# Add the given directory to the search path.
#        Input - head of the stack
#        Note - Win9x systems requires reboot

Function AddToPath
  Exch $0
  Push $1
  Push $2
  Push $3

  # don't add if the path doesn't exist
  IfFileExists "$0\*.*" "" AddToPath_done

  ReadEnvStr $1 PATH
  Push "$1;"
  Push "$0;"
  Call StrStr
  Pop $2
  StrCmp $2 "" "" AddToPath_done
  Push "$1;"
  Push "$0\;"
  Call StrStr
  Pop $2
  StrCmp $2 "" "" AddToPath_done
  GetFullPathName /SHORT $3 $0
  Push "$1;"
  Push "$3;"
  Call StrStr
  Pop $2
  StrCmp $2 "" "" AddToPath_done
  Push "$1;"
  Push "$3\;"
  Call StrStr
  Pop $2
  StrCmp $2 "" "" AddToPath_done

  Call IsNT
  Pop $1
  StrCmp $1 1 AddToPath_NT
    	# Not on NT
    StrCpy $1 $WINDIR 2
    FileOpen $1 "$1\autoexec.bat" a
    FileSeek $1 -1 END
    FileReadByte $1 $2
    IntCmp $2 26 0 +2 +2 # DOS EOF
      FileSeek $1 -1 END # write over EOF
    	FileWrite $1 "$\r$\nSET PATH=%PATH%#$3$\r$\n"
    FileClose $1
    SetRebootFlag true
    Goto AddToPath_done
AddToPath_NT:
    ReadRegStr $1 HKCU "Environment" "PATH"
    StrCpy $2 $1 1 -1 # copy last char
    	StrCmp $2 "#" 0 +2 # if last char == #
      StrCpy $1 $1 -1 # remove last char
    StrCmp $1 "" AddToPath_NTdoIt
      	StrCpy $0 "$1#$0"
AddToPath_NTdoIt:
      WriteRegExpandStr HKCU "Environment" "PATH" $0
      SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
AddToPath_done:
    Pop $3
    Pop $2
    Pop $1
    Pop $0
FunctionEnd

# Remove a given directory from the path
#     Input: head of the stack

Function un.RemoveFromPath
  Exch $0
  Push $1
  Push $2
  Push $3
  Push $4
  Push $5
  Push $6

  IntFmt $6 "%c" 26 # DOS EOF

  Call un.IsNT
  Pop $1
  StrCmp $1 1 unRemoveFromPath_NT
    	# Not on NT
    StrCpy $1 $WINDIR 2
    FileOpen $1 "$1\autoexec.bat" r
    GetTempFileName $4
    FileOpen $2 $4 w
    GetFullPathName /SHORT $0 $0
    StrCpy $0 "SET PATH=%PATH%;$0"
    Goto unRemoveFromPath_dosLoop
unRemoveFromPath_dosLoop:
      FileRead $1 $3
      StrCpy $5 $3 1 -1 # read last char
      StrCmp $5 $6 0 +2 # if DOS EOF
        StrCpy $3 $3 -1 # remove DOS EOF so we can compare
      StrCmp $3 "$0$\r$\n" unRemoveFromPath_dosLoopRemoveLine
      StrCmp $3 "$0$\n" unRemoveFromPath_dosLoopRemoveLine
      StrCmp $3 "$0" unRemoveFromPath_dosLoopRemoveLine
      StrCmp $3 "" unRemoveFromPath_dosLoopEnd
      FileWrite $2 $3
      Goto unRemoveFromPath_dosLoop
unRemoveFromPath_dosLoopRemoveLine:
        SetRebootFlag true
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
    ReadRegStr $1 HKCU "Environment" "PATH"
    StrCpy $5 $1 1 -1 # copy last char
    StrCmp $5 ";" +2 # if last char != ;
      StrCpy $1 "$1;" # append ;
    Push $1
    Push "$0;"
    	Call un.StrStr # Find `$0;` in $1
    	Pop $2 # pos of our dir
    StrCmp $2 "" unRemoveFromPath_done
      	# else, it is in path
      # $0 - path to add
      # $1 - path var
      StrLen $3 "$0;"
      StrLen $4 $2
      StrCpy $5 $1 -$4 # $5 is now the part before the path to remove
      StrCpy $6 $2 "" $3 # $6 is now the part after the path to remove
      StrCpy $3 $5$6

      StrCpy $5 $3 1 -1 # copy last char
      StrCmp $5 ";" 0 +2 # if last char == ;
        StrCpy $3 $3 -1 # remove last char

      WriteRegExpandStr HKCU "Environment" "PATH" $3
      SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
unRemoveFromPath_done:
    Pop $6
    Pop $5
    Pop $4
    Pop $3
    Pop $2
    Pop $1
    Pop $0
FunctionEnd

# StrStr
# input, top of stack = string to search for
#        top of stack-1 = string to search in
# output, top of stack (replaces with the portion of the string remaining)
# modifies no other variables.
#
# Usage:
#   Push "this is a long ass string"
#   Push "ass"
#   Call StrStr
#   Pop $R0
#  ($R0 at this point is "ass string")

!macro StrStr un
Function ${un}StrStr
	Exch $R1 # st=haystack,old$R1, $R1=needle
  	Exch    # st=old$R1,haystack
  	Exch $R2 # st=old$R1,old$R2, $R2=haystack
  Push $R3
  Push $R4
  Push $R5
  StrLen $R3 $R1
  StrCpy $R4 0
  	# $R1=needle
  	# $R2=haystack
  	# $R3=len(needle)
  	# $R4=cnt
  	# $R5=tmp
loop:
    StrCpy $R5 $R2 $R3 $R4
    StrCmp $R5 $R1 done
    StrCmp $R5 "" done
    IntOp $R4 $R4 + 1
    Goto loop
done:
  StrCpy $R1 $R2 "" $R4
  Pop $R5
  Pop $R4
  Pop $R3
  Pop $R2
  Exch $R1
FunctionEnd
!macroend
!insertmacro StrStr ""
!insertmacro StrStr "un."

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
  SetOutPath $INSTDIR
!ifdef NONFREE
		File ..\..\readme-csound5-complete.txt
!else
  File ..\..\readme-csound5.txt
!endif
		File ..\..\etc\.csoundrc
  File ..\..\INSTALL
  SetOutPath $INSTDIR\bin
!ifdef FLOAT
		File ..\..\csound32.dll.5.1
!else
  File ..\..\csound64.dll.5.1
!endif
  File ..\..\csnd.dll
  File ..\..\_jcsound.dll
  File ..\..\CsoundAC.dll
!ifdef NONFREE
		File ..\..\CsoundVST.dll
		File ..\..\*.exe
!else
		File /x CsoundVST.exe ..\..\*.exe
!endif
   File ..\..\tclcsound.dll
  File ..\..\csoundapi~.dll
  File D:\utah\msys\1.0\local\bin\mgwfltknox-1.1.dll
  File D:\utah\msys\1.0\local\bin\mgwfltknox_images-1.1.dll
  File D:\utah\opt\libsndfile-1_0_17\libsndfile-1.dll
  File D:\utah\opt\lazzarini\portaudio.dll
  File D:\utah\opt\csound5\bin\fluidsynth.dll
  File D:\utah\opt\portmidi\pm_win\*.dll
  File D:\utah\opt\LuaJIT-1.1.2\src\luajit.exe
  File D:\utah\opt\LuaJIT-1.1.2\src\lua51.dll
  File D:\utah\home\mkg\projects\liblo\src\liblo.dll
  File D:\utah\opt\pthreads\Pre-built.2\lib\pthreadGC2.dll
  File D:\utah\opt\fftw-3.1.2\.libs\libfftw3-3.dll
  File ..\..\csound.def
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
  File ..\..\tutorial\tutorial.pdf
  File ..\..\tutorial\*.csd
  File ..\..\tutorial\*.py
  File ..\..\tutorial\tutorial3.cpr
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
		SetOutPath $INSTDIR\examples\python_demo
			File /x *.wav /x *.orc /x *.pyc /x *.sco ..\..\examples\python_demo\*.*
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
!ifdef NONFREE
  SetOutPath $INSTDIR\samples
  File /r ..\..\samples\*
		File /r ..\..\Opcodes\stk\rawwaves\*.raw
!endif
	# Then we do output paths for Python stuff.
	SetOutPath $INSTDIR\$PYTHON_OUTPUT_PATHbin
		File ..\..\_CsoundAC.pyd
		File ..\..\CsoundAC.py
		FILE ..\..\_csnd.pyd
		File ..\..\csnd.py
		File ..\..\CompositionBase.py
	# Then we do opcodes, which are a special case with respect to Python and non-free software.
	SetOutPath $INSTDIR\${OPCODEDIR_VAL}
!ifdef PYTHON_AVAILABLE
	!ifdef NONFREE
		DetailPrint "Python available, free and non-free software."
		File /x csound*.dll* \
		/x *.pyd \       
		/x libsndfile-1.dll \
		/x portaudio.dll* \
		/x tclcsound.dll \
		/x csoundapi~.dll \
		/x pm_midi.dll \
		..\..\py.dll \
		..\..\*.dll \
		..\..\frontends\csladspa\csladspa.dll
	!else
		DetailPrint "Python available, only free software."
		File /x csound*.dll* \
		/x stk.dll \
		/x vst4cs.dll \
		/x *.pyd \       
		/x libsndfile-1.dll \
		/x portaudio.dll* \
		/x tclcsound.dll \
		/x csoundapi~.dll \
		/x pm_midi.dll \
		..\..\*.dll \
		..\..\frontends\csladspa\csladspa.dll
	!endif
!else
	!ifdef NONFREE
		DetailPrint "Python not available, free and non-free software."
		File /x csound*.dll* \
		/x *.pyd \       
		/x libsndfile-1.dll \
		/x portaudio.dll* \
		/x tclcsound.dll \
		/x csoundapi~.dll \
		/x pm_midi.dll \
		..\..\*.dll \
		..\..\frontends\csladspa\csladspa.dll
	!else
		DetailPrint "Python not available, only free software."
		File /x csound*.dll* \
		/x stk.dll \
		/x py.dll \
		/x vst4cs.dll \
		/x *.pyd \       
		/x libsndfile-1.dll \
		/x portaudio.dll* \
		/x tclcsound.dll \
		/x csoundapi~.dll \
		/x pm_midi.dll \
		..\..\*.dll \
		..\..\frontends\csladspa\csladspa.dll
	!endif
!endif
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
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\CsoundVST.lnk" "$INSTDIR\bin\CsoundVST.exe" "" "" "" "" "" "CsoundVST GUI"
!endif
	CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\License.lnk" "$INSTDIR\doc\readme-csound5-complete.txt" "" "" "" "" "" "Csound README"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Manual.lnk" "$INSTDIR\doc\manual\index.html" "" "" "" "" "" "Csound manual"
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
