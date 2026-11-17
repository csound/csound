<CsTest>
description = "i() with no argument, assigned to a variable, is an error and not a crash"

[expect]
exit = "nonzero"
stderr = ["Unable to find opcode entry for 'i'"]
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
  value:i = i()
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
