<CsTest>
description = "instanced loop gives each iteration its own oscillator"

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
  freqs:i[] fillarray 220, 330, 440, 550
  sum:a = 0
  for f:i in freqs instanced do
    sum += oscili(0.2, f)
  od
  want:a = oscili(0.2, 220) + oscili(0.2, 330) + oscili(0.2, 440) + oscili(0.2, 550)
  assertSameSignal(sum, want, "oscillator sum")
  assertNotSilent(want, "oscillator sum")
endin
</CsInstruments>
<CsScore>
i 1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
