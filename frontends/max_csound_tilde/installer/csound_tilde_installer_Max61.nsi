; csound_tilde.nsi
;
; Windows installer for csound~.
;
;----------------------------------------------------------------

    !include MUI2.nsh
    
    !define COMPANY "Csound"
    !define PRODUCT "csound~"
	; Change FTYPE and OPCODEDIR in tandem
    !define FTYPE "double" ; float or double
	!define OPCODE6DIR "OPCODE6DIR64" ; OPCODEDIR or OPCODEDIR64
    !define SOURCE_MXE "win\${PRODUCT}_${FTYPE}.mxe"
	!define SOURCE_MXE_NO_PATH "${PRODUCT}_${FTYPE}.mxe"
    !define TARGET_MXE "${PRODUCT}.mxe"

;----------------------------------------------------------------
; General

    ; The name of the installer
    Name "csound~_v1.1.0_win_${FTYPE}_Max61"
    
    ; The file to write
    OutFile "csound~_v1.1.0_win_${FTYPE}_Max61.exe"
    
    ; The default installation directory
    InstallDir "$DOCUMENTS\Max\Packages\csound~_v1.1.0_${FTYPE}"
    
    ; Request application privileges for Windows Vista/7
    ;RequestExecutionLevel user

;----------------------------------------------------------------
; Interface Settings

  !define MUI_ABORTWARNING

;----------------------------------------------------------------
; Pages

    !insertmacro MUI_PAGE_LICENSE "Copyright_Permissions.txt"
    !insertmacro MUI_PAGE_COMPONENTS
    !insertmacro MUI_PAGE_DIRECTORY
    !insertmacro MUI_PAGE_INSTFILES

;----------------------------------------------------------------
;Languages
 
    !insertmacro MUI_LANGUAGE "English"

;----------------------------------------------------------------
; Variables

    Var Csound5_plugin_dir
    Var FluidOpcodesDll
	Var Max6_dir
    Var doc_dir
    Var examples_dir
    Var src_dir
	Var help_dir
	Var externals_dir
	
;----------------------------------------------------------------
; Functions

    Function GetMaxDirs
		ReadRegStr $Max6_dir HKLM `Software\Cycling '74\Max 6.1` `Installed Location`
		IfFileExists $Max6_dir\*.* Max_exists 0
		MessageBox MB_OK "No Max 6.1 installations detected."
		Abort
		Max_exists:		
    FunctionEnd
	
    Function .onInit
        ClearErrors
        Call GetMaxDirs
        ReadRegStr $Csound5_plugin_dir HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" ${OPCODEDIR}
        StrCmp $Csound5_plugin_dir "" 0 continue_installing
        StrCpy $1 "Csound5 ${FTYPE}s version not detected. $\r$\n$\r$\nCsound5 must be installed before using csound~. Continue with installation anyways?"
        MessageBox MB_YESNO $1 IDYES continue_installing IDNO stop_installing
    stop_installing:
        Abort
    continue_installing:
        ; Delete previously renamed fluidOpcodes.dll.backup
        StrCpy $FluidOpcodesDll "$Csound5_plugin_dir\fluidOpcodes.dll.backup"
        Delete $FluidOpcodesDll
        ; Check for fluidOpcodes.dll
        StrCpy $FluidOpcodesDll "$Csound5_plugin_dir\fluidOpcodes.dll"
        IfFileExists $FluidOpcodesDll 0 dont_rename_fluidopcodes_dll
        StrCpy $1 "Detected $FluidOpcodesDll. $\r$\n$\r$\nThis may cause csound~ to crash. $\r$\n$\r$\nWould you like me to disable (rename) it?"
        MessageBox MB_YESNO $1 IDYES rename_fluidopcodes_dll IDNO dont_rename_fluidopcodes_dll
    rename_fluidopcodes_dll:    
        Rename $FluidOpcodesDll "$Csound5_plugin_dir\fluidOpcodes.dll.backup"
    dont_rename_fluidopcodes_dll:
    FunctionEnd
    
Function .onInstSuccess
    MessageBox MB_YESNO "csound~ successfully installed! You can now use it in Max. Would you like to view the files?" IDNO NoReadme
      Exec 'explorer.exe $INSTDIR' ; view readme or whatever, if you want.
    NoReadme:
  FunctionEnd    

;----------------------------------------------------------------
; Sections

    Section "!csound~" Sec_Externals
        SectionIn RO ; Installing external is not optional.
		CreateDirectory $INSTDIR
		StrCpy $externals_dir "$INSTDIR\externals"
		SetOutPath $externals_dir
		File ${SOURCE_MXE}
		Rename ${SOURCE_MXE_NO_PATH} ${TARGET_MXE}
    SectionEnd
    
    
    Section "HelpFiles" Sec_Help
		StrCpy $help_dir "$INSTDIR\help"
		SetOutPath $help_dir
		File /r help\*.*
    SectionEnd
    
    
    Section "Documentation and Examples" Sec_Docs    
        StrCpy $doc_dir "$INSTDIR\docs"
        StrCpy $examples_dir "$INSTDIR\examples"
        SetOutPath $doc_dir
        File /r docs\*.*
        SetOutPath $examples_dir
        File /r examples\*.*
        SetOutPath $INSTDIR
        File "LICENSE.TXT"
    SectionEnd
    
    
    Section "Source Code" Sec_Source
        StrCpy $src_dir "$INSTDIR\source"
        SetOutPath $src_dir
        File /r src\*.*
    SectionEnd    

;----------------------------------------------------------------
; Descriptions

    LangString DESC_Sec_Externals ${LANG_ENGLISH} "csound~.mxe Max6 external."
    LangString DESC_Sec_Help ${LANG_ENGLISH} "csound~.maxhelp and associated CSD files."
    LangString DESC_Sec_Docs ${LANG_ENGLISH} "Documentation and example patches."
    LangString DESC_Sec_Source ${LANG_ENGLISH} "Source code and project files."
    
    !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
        !insertmacro MUI_DESCRIPTION_TEXT ${Sec_Externals} $(DESC_Sec_Externals)
        !insertmacro MUI_DESCRIPTION_TEXT ${Sec_Help} $(DESC_Sec_Help)
        !insertmacro MUI_DESCRIPTION_TEXT ${Sec_Docs} $(DESC_Sec_Docs)
        !insertmacro MUI_DESCRIPTION_TEXT ${Sec_Source} $(DESC_Sec_Source)
    !insertmacro MUI_FUNCTION_DESCRIPTION_END
    
;----------------------------------------------------------------    
