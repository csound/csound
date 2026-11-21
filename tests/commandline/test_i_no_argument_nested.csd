<CsTest>
description = "i() with no argument, inside another call, is an error and not a crash"

[expect]
exit = "nonzero"
stderr = ["opcode 'i' for expression with arg types"]
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
  x:i = strlen(i())
  print x
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
