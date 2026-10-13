<CsTest>
description = "Sliding pvsanal rejects invalid window sizes before rounding or allocation"

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
i "CheckSize" 0 .01 0
i "CheckSize" 0 .01 -1
i "CheckSize" 0 .01 1e20
e
</CsScore>
</CsoundSynthesizer>
