<CsTest>
description = "instanced loops are rejected in global code"

[expect]
exit = "nonzero"
stderr = ["instanced loops are only allowed inside instruments and UDOs"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
0dbfs = 1

freqs@global:i[] fillarray 220, 330
for f:i in freqs instanced do
  prints("%f\n", f)
od
</CsInstruments>
<CsScore>
i 1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
