<CsTest>
description = "overlapping notes keep separate instanced loop state"

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
  freqs:i[] fillarray p4, p4 * 1.5
  sum:a = 0
  for f:i in freqs instanced do
    sum += oscili(0.2, f)
  od
  want:a = oscili(0.2, p4) + oscili(0.2, p4 * 1.5)
  assertSameSignal(sum, want, "overlapping notes")
  assertNotSilent(want, "overlapping notes")
endin
</CsInstruments>
<CsScore>
i 1 0 0.5 220
i 1 0.1 0.5 330
i 1 0.2 0.5 440
e
</CsScore>
</CsoundSynthesizer>
