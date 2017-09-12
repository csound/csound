@rem This script expects the Microsoft Library Manager (LIB) and COFF Binary
@rem File Dumper (DUMPBIN) to be on your Windows PATH. Depending on your version
@rem of Visual Studio, you can add these executables to your PATH by running,
@rem for example,
@rem   set PATH=%PATH%;%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\bin
@rem To create a csound64.lib file for 64-bit Windows, enter in Command Prompt
@rem   make_import_library
@rem To create a csound64.lib file for 32-bit Windows, enter in Command Prompt
@rem   make_import_library /machine:x86

echo LIBRARY csound64.dll > csound64.def && echo EXPORTS >> csound64.def
for /F "skip=19 tokens=4" %%l in ('dumpbin /exports csound-mingw64/csound64.dll') do @echo %%l >> csound64.def
lib /def:csound64.def /machine:x64 %*
