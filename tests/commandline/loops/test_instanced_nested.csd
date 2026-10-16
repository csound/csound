<CsTest>
description = "nested instanced loops give every inner iteration its own instance"

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
  freqs:i[] fillarray 220, 330
  ratios:i[] fillarray 1, 2
  sum:a = 0
  for f:i in freqs instanced do
    for r:i in ratios instanced do
      sum += oscili(0.1, f * r)
    od
  od
  want:a = oscili(0.1, 220 * 1) + oscili(0.1, 220 * 2)
  want += oscili(0.1, 330 * 1) + oscili(0.1, 330 * 2)
  assertSameSignal(sum, want, "nested loops")
  assertNotSilent(want, "nested loops")
endin
</CsInstruments>
<CsScore>
i 1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
