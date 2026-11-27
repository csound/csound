<CsTest>
description = "distort rejects non-finite input even when the detector is frozen"
[expect]
exit = "nonzero"
stderr = ["distort: input level is out of range"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1
giShape ftgen 0, 0, 8, -2, 1, 1, 1, 1, 1, 1, 1, 1
instr 1
  aInput init sqrt(-1)
  aOutput distort aInput, 1, giShape, 0
endin
instr 2
  aInput init exp(1000)
  aOutput distort aInput, 1, giShape, 0
endin
</CsInstruments>
<CsScore>
i 1 0 .001
i 2 0 .001
e
</CsScore>
</CsoundSynthesizer>
