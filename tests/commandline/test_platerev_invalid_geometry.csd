<CsTest>
description = "platerev rejects geometry that cannot fit its grid"

[expect]
exit = "nonzero"
stderr = [
  "platerev: aspect ratio must be in (0, 1]",
  "platerev: decay time is too small",
  "platerev: input frequency must be finite and radius must be in (-1, 1)",
]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1

instr 1
  aImpulse = mpulse(1, 0)
  aOut platerev 1, 2, 1, p4, 1, p5, 0.001, aImpulse
endin
</CsInstruments>
<CsScore>
f 1 0 -3 -2 0 1000 0
f 2 0 -3 -2 0 0 0
i 1 0 0.01 0 1
i 1 0.02 0.01 1 0.0000000001
i 1 0.04 0.01 1 1
</CsScore>
</CsoundSynthesizer>
