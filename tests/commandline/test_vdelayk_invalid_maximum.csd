<CsTest>
description = "vdelayk rejects invalid sizes before allocation"

[expect]
exit = "nonzero"
stderr = ["vdelayk: invalid maximum delay"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
instr 1
  kOut vdelayk 1, 0, p4
endin
</CsInstruments>
<CsScore>
i 1 0 .1 -1
i 1 0 .1 1e30
</CsScore>
</CsoundSynthesizer>
