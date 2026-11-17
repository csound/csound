<CsTest>
description = "i() of an undefined variable, assigned to a variable, is an ordinary compile error"

[expect]
exit = "nonzero"
stderr = ["Variable type for kUndefined could not be determined"]
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
  value:i = i(kUndefined)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
