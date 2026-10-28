<CsTest>
description = "an instrument can shadow an opcode name for schedule"

[expect]
exit = 0
output = ["hello"]
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

instr reverb
  print "hello"
endin

schedule reverb, 0, 1

</CsInstruments>
<CsScore>
f 0 1
</CsScore>
</CsoundSynthesizer>
