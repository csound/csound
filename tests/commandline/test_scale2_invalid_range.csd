<CsTest>
description = "scale2 rejects equal or reversed input bounds at performance time"
[expect]
exit = "nonzero"
stderr = ["scale2: input maximum must exceed minimum"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 128
nchnls = 1
0dbfs = 1
instr 1
  kCycle init 0
  kMax = (kCycle == 0 ? 1 : p4)
  kOut scale2 .5, 0, 1, 0, kMax
  kCycle += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .5 0
i 1 0 .5 -1
</CsScore>
</CsoundSynthesizer>
