<CsTest>
description = "instanced loops work inside a UDO"

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

opcode Chord(root:i):a
  ratios:i[] fillarray 1, 1.25, 1.5
  sum:a = 0
  for r:i in ratios instanced do
    sum += oscili(0.2, root * r)
  od
  xout sum
endop

instr 1
  got:a = Chord(220)
  want:a = oscili(0.2, 220 * 1) + oscili(0.2, 220 * 1.25) + oscili(0.2, 220 * 1.5)
  assertSameSignal(got, want, "loop in UDO")
  assertNotSilent(want, "loop in UDO")
endin
</CsInstruments>
<CsScore>
i 1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
