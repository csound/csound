<CsTest>
description = "zakinit rejects sizes before converting or allocating"
[expect]
exit = "nonzero"
stderr = ["zakinit: sizes out of range"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1
instr 1
zakinit p4, p5
endin
</CsInstruments>
<CsScore>
i 1 0 .01 0 1
i 1 0 .01 1 -1
i 1 0 .01 1e30 1
i 1 0 .01 1 1e30
e
</CsScore>
</CsoundSynthesizer>
