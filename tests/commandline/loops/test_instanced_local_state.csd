<CsTest>
description = "variables declared in an instanced loop body keep separate state per iteration"

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
  for f:i in freqs instanced do
    fade:k init 0
    fade = fade * 0.99 + 0.01
    sum += moogladder(vco2(0.2, f), 2000, 0.3) * fade
  od

  fade1:k init 0
  fade1 = fade1 * 0.99 + 0.01
  fade2:k init 0
  fade2 = fade2 * 0.99 + 0.01
  fade3:k init 0
  fade3 = fade3 * 0.99 + 0.01
  want:a = moogladder(vco2(0.2, 220), 2000, 0.3) * fade1
  want += moogladder(vco2(0.2, 330), 2000, 0.3) * fade2
  want += moogladder(vco2(0.2, 440), 2000, 0.3) * fade3
  assertSameSignal(sum, want, "per-iteration state")
  assertNotSilent(want, "per-iteration state")
endin
</CsInstruments>
<CsScore>
i 1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
