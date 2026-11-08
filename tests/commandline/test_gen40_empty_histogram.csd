<CsTest>
description = "GEN40 rejects a histogram with no weight"

[expect]
exit = "nonzero"
stderr = ["GEN40: histogram total must be positive and finite"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1
giSource ftgen 1, 0, 4, -2, 0, 0, 0, 0
giResult ftgen 2, 0, 8, -40, giSource
</CsInstruments>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
