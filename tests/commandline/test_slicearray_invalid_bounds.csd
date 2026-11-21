<CsTest>
description = "slicearray rejects invalid source bounds, including after the source shrinks"
[expect]
exit = "nonzero"
stderr = ["slicearray: invalid slice bounds", "slice larger than original size"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
instr NegativeStart
  iInput[] fillarray 1, 2, 3, 4
  iOutput[] slicearray iInput, -1, 2
endin
instr ShrinkingSource
  kInput[] fillarray 1, 2, 3, 4
  kOutput[] slicearray kInput, 1, 3
  trim kInput, 2
endin
</CsInstruments>
<CsScore>
i "NegativeStart" 0 .015625
i "ShrinkingSource" .03125 .03125
e
</CsScore>
</CsoundSynthesizer>
