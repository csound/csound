<CsTest>
description = "GEN21 rejects missing beta and Weibull shape parameters"

[expect]
exit = "nonzero"
stderr = ["ftable 1: Wrong number of input arguments", "ftable 2: Wrong number of input arguments"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 1

instr 1
  iTable ftgen 1, 0, 16, 21, 9, 1, 2
endin

instr 2
  iTable ftgen 2, 0, 16, 21, 10, 1
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 2 0 0.01
e
</CsScore>
</CsoundSynthesizer>
