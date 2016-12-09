This folder contains a minimal build of Csound for 64-bit Windows. If you're
trying to install Csound along with its documentation, default frontend
[CsoundQt](https://csoundqt.github.io), and other extras, download and run one
of the setup executables at https://github.com/csound/csound/releases. The names
of the setup executables begin with Setup_Csound6_x64_.

It's a good idea, but not essential, to move the bin, include, and lib folders
to roughly where Csound's setup executables install them. To do this, open an
administrator Command Prompt in this folder and enter

    set CsoundPath=%ProgramFiles%\Csound6_x64
    if not exist "%CsoundPath%\NUL" mkdir "%CsoundPath%"
    move bin "%CsoundPath%"
    move include "%CsoundPath%"
    move lib "%CsoundPath%"

To easily run the executables in the bin folder, you must add the bin folder to
your Windows path and create an OPCODE6DIR64 environment variable. To do this on
Windows 10:

1. Right-click the Start button, and then select Run from the menu that appears.

2. Type

       sysdm.cpl ,3

   (note the space between sysdm.cpl and ,3) in the Open field and press Enter.
The System Properties window appears with the Advanced tab selected.

3. Click Environment Variables at the bottom of the window.

4. In system variables, select Path, and then click Edit. Click New, enter the
path to the bin folder, and then click OK. If you moved the bin folder to where
Csound's setup executables install it, the path of the bin folder is probably
C:\Program Files\Csound6_x64\bin.

5. In system variables, click New to create an environment variable named
OPCODE6DIR64 with a value of the path to the csound\plugins64-6.0 folder within
the lib folder, and then click OK. If you moved the lib folder to where Csound's
setup executables install it, this path is probably
C:\Program Files\Csound6_x64\lib\csound\plugins64-6.0.

This package contains an import library for users of Visual Studio. However, if
for some reason you wish to create your own you may do so using the following steps. 

Open an administrator Command Prompt in the bin folder and enter:

    set PATH=%PATH%;%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\bin
    echo LIBRARY csound64.dll > csound64.def && echo EXPORTS >> csound64.def
    for /F "skip=19 tokens=4" %G in ('dumpbin /exports csound64.dll') do @echo %G >> csound64.def
    if not exist ..\lib\NUL mkdir ..\lib
    lib /def:csound64.def /out:..\lib\csound64.lib /machine:x64
