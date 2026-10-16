<CsTest>
description = "instanced loop over a k-rate array follows values that change during performance"

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
  lfo:k = oscili(1, 3)
  freqs:k[] fillarray 220, 330
  freqs[0] = 220 + 20 * lfo
  freqs[1] = 330 - 20 * lfo
  sum:a = 0
  for f:k in freqs instanced do
    sum += oscili(0.2, f)
  od
  want:a = oscili(0.2, 220 + 20 * lfo) + oscili(0.2, 330 - 20 * lfo)
  assertSameSignal(sum, want, "k-rate array")
  assertNotSilent(want, "k-rate array")
endin
</CsInstruments>
<CsScore>
i 1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
