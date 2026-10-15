<CsTest>
description = "only instanced may appear between the array and do"

[expect]
exit = "nonzero"
stderr = ["expected 'instanced' or 'do'"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
0dbfs = 1

instr 1
  freqs:i[] fillarray 220, 330
  for f:i in freqs parallel do
    prints("%f\n", f)
  od
endin
</CsInstruments>
<CsScore>
i 1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
