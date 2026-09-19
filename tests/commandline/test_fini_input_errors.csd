<CsTest>
description = "fini reports invalid input formats, skip counts and text"

[expect]
exit = "nonzero"
stderr = ["fini: format must be 0, 1 or 2", "fini: invalid skip frame count", "fini: invalid numeric data", "error opening file"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  fprints "fini_invalid.dat", "not-a-number"
  ficlose "fini_invalid.dat"
endin
instr 2
  iValue init 0
  fini "fini_invalid.dat", p5, p4, iValue
endin
instr 3
  iValue init 0
  fini "fini_nonexistent_directory/missing.txt", 0, 1, iValue
endin
</CsInstruments>
<CsScore>
i1 0 .001
i2 .01 .001 3 0
i2 .01 .001 1 -1
i2 .01 .001 1 0
i3 .01 .001
</CsScore>
</CsoundSynthesizer>
