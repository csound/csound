<CsTest>
description = "expon still rejects zero endpoints and opposite signs"
[expect]
exit = "nonzero"
stderr = ["arg1 is zero", "arg2 is zero", "unlike signs"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 1
nchnls = 1

instr InvalidCurve
  kCurve expon p4, 1, p5
endin
</CsInstruments>
<CsScore>
i "InvalidCurve" 0 .1 0 1
i "InvalidCurve" 0 .1 1 0
i "InvalidCurve" 0 .1 -1 1
i "InvalidCurve" 0 .1 1 -1
e
</CsScore>
</CsoundSynthesizer>
