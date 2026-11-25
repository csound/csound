<CsTest>
description = "swtanal rejects unsupported boundaries and windowless overlap-add"

[expect]
exit = "nonzero"
stderr = ["swtanal: only periodic boundary is supported", "swtanal: overlap-add requires a window"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  aInput = 0.1
  w:swt swtanal aInput, p4, p5, 2, 4, 4, p6, p7, p8
endin
</CsInstruments>
<CsScore>
i 1 0 0.01 64 32 1 1 2
i 1 0 0.01 64 64 0 0 2
e
</CsScore>
</CsoundSynthesizer>
