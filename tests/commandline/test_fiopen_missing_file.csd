<CsTest>
description = "fiopen reports a failed open instead of returning a handle"

[expect]
exit = "nonzero"
stderr = ["error opening file"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  iFile fiopen "fiopen_nonexistent_directory/missing.txt", 1
endin
</CsInstruments>
<CsScore>
i1 0 .01
</CsScore>
</CsoundSynthesizer>
