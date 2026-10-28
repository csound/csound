<CsTest>
description = "an instrument keeps a non-opcode global name and stays schedulable by string"

[expect]
exit = 0
output = ["refer to the instrument by string name", "target"]
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

reverb@global:i = 5

instr reverb
  print "target"
endin

schedule "reverb", 0, 1

</CsInstruments>
<CsScore>
f 0 1
</CsScore>
</CsoundSynthesizer>
