<CsTest>
description = "instanced loop body reads outer a-, k- and i-rate variables"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
0dbfs = 1

#include "libinstanced.orc"

instr 1
  source:a = vco2(0.3, 110)
  base:k = 400
  resonance:i = 0.4
  ratios:i[] fillarray 1, 2, 4
  sum:a = 0
  for r:i in ratios instanced do
    sum += moogladder(source, base * r, resonance)
  od
  want:a = moogladder(source, base * 1, resonance)
  want += moogladder(source, base * 2, resonance)
  want += moogladder(source, base * 4, resonance)
  assertSameSignal(sum, want, "captured inputs")
  assertNotSilent(want, "captured inputs")
endin
</CsInstruments>
<CsScore>
i 1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
