<CsTest>
description = "Sliding pvsanal rejects the rounded int32 limit before conversion or allocation"

[expect]
exit = "nonzero"
stderr = ["Invalid window size"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
instr CheckSize
 aInput = 0
 fAnalysis pvsanal aInput, 64, 1, p4, 6
endin
</CsInstruments>
<CsScore>
i "CheckSize" 0 .01 2147483648
e
</CsScore>
</CsoundSynthesizer>
