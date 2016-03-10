@echo CREATING MICROSOFT IMPORT LIBRARY FOR CSOUND64.DLL FOR X64
dumpbin /exports csound-mingw64/csound64.dll > exports.txt
@echo LIBRARY CSOUND64 >> csound64.def
@echo EXPORTS >> csound64.def
FOR /F "skip=19 tokens=4" %%l in (exports.txt) do @echo %%l >> csound64.def
lib /def:csound64.def /out:csound64.lib /machine:x64
