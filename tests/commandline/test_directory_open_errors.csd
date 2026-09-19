<CsTest>
description = "directory reports missing paths and paths that are not directories"
[expect]
exit = "nonzero"
stderr = ["cannot open directory"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  fprints "directory_regular_file.txt", "regular file"
  ficlose "directory_regular_file.txt"
endin
instr 2
  SNames[] directory "directory_missing_parent/child"
endin
instr 3
  SNames[] directory "directory_regular_file.txt", ".txt"
endin
instr 4
  SNames[] directory ""
endin
</CsInstruments>
<CsScore>
i1 0 .001
i2 .01 .001
i3 .01 .001
i4 .01 .001
</CsScore>
</CsoundSynthesizer>
