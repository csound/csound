<CsTest>
description = "expected failure: GEN42 probability total overflows"

[expect]
exit = "nonzero"
stderr = ["Gen42: probability total must be finite"]
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
giInvalid ftgen 1, 0, -8, -42, \
  10, 12, giWeight, 20, 22, giWeight, 30, 32, giWeight
</CsInstruments>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
