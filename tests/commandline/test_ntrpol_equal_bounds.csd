<CsTest>
description = "ntrpol rejects a zero-width interpolation interval at every rate"

[expect]
exit = "nonzero"
stderr = ["opcode ntrpol.i", "opcode ntrpol.k", "opcode ntrpol.a", "Min and max the same"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1

instr InitRate
  iValue ntrpol 2, 6, p4, p5, p5
endin

instr ControlRate
  kValue ntrpol 2, 6, p4, p5, p5
endin

instr AudioRate
  aFirst = 2
  aSecond = 6
  aValue ntrpol aFirst, aSecond, p4, p5, p5
endin
</CsInstruments>
<CsScore>
; Equal bounds must fail whether the point equals the bound or differs.
; The i-rate form previously returned NaN or infinity, respectively.
i "InitRate"    0 0.25 0 0
i "InitRate"    0 0.25 5 4
i "ControlRate" 0 0.25 0 0
i "ControlRate" 0 0.25 5 4
i "AudioRate"   0 0.25 0 0
i "AudioRate"   0 0.25 5 4
e
</CsScore>
</CsoundSynthesizer>
