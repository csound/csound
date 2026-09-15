<CsTest>
description = "expected failure: GEN41 probability total overflows"

[expect]
exit = "nonzero"
stderr = ["Gen41: probability total must be finite"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

; Each weight is finite in both float and double builds; their sum overflows.
giWeight = $M_MAX_VALUE / 2
giInvalid ftgen 1, 0, -8, -41, \
  10, giWeight, 20, giWeight, 30, giWeight
</CsInstruments>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
