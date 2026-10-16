<CsTest>
description = "instanced loop body can write a global variable directly"

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

bus@global:a init 0

instr 1
  freqs:i[] fillarray 220, 330
  bus = 0
  for f:i in freqs instanced do
    bus += oscili(0.2, f)
  od
  want:a = oscili(0.2, 220) + oscili(0.2, 330)
  assertSameSignal(bus, want, "global target")
  assertNotSilent(want, "global target")
endin
</CsInstruments>
<CsScore>
i 1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
