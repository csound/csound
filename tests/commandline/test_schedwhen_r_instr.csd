<CsTest>
description = "schedwhen accepts a numeric :r instrument"

[expect]
exit = 0
output = ["target"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>

sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1

instr 1
  print "target"
endin

instr 2
  schedwhen 1, nchnls, 0, 0.05
endin

</CsInstruments>
<CsScore>
i 2 0 0.1
f 0 1
</CsScore>
</CsoundSynthesizer>
