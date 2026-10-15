<CsTest>
description = "break is rejected in an instanced loop"

[expect]
exit = "nonzero"
stderr = ["break and continue are not allowed in instanced loops"]
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
  sum:a = 0
  for f:i in freqs instanced do
    sum += oscili(0.2, f)
    break
  od
endin
</CsInstruments>
<CsScore>
i 1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
