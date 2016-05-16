@echo CREATING MICROSOFT IMPORT LIBRARY FOR CSOUND64.DLL
dumpbin /exports %CSOUND_HOME%\csound64.dll > exports.txt
@echo LIBRARY CSOUND64 >> %CSOUND_HOME%\csound64.def
@echo EXPORTS >> %CSOUND_HOME%\csound64.def
FOR /F "skip=19 tokens=4" %%l in (exports.txt) do @echo %%l >> %CSOUND_HOME%\csound64.def
lib /def:%CSOUND_HOME%\csound64.def /out:%CSOUND_HOME%\csound64.lib /machine:x86
