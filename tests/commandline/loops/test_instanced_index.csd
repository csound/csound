<CsTest>
description = "instanced loop with an index variable"

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
  freqs:i[] fillarray 220, 330, 440
  sum:a = 0
  for f:i, n:i in freqs instanced do
    sum += oscili((n + 1) * 0.125, f)
  od
  want:a = oscili(0.125, 220) + oscili(0.25, 330) + oscili(0.375, 440)
  assertSameSignal(sum, want, "index variable")
  assertNotSilent(want, "index variable")
endin
</CsInstruments>
<CsScore>
i 1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
