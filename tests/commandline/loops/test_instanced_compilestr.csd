<CsTest>
description = "instanced loops in code compiled during performance get their own UDOs"

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
  sum:a = 0
  for f:i in freqs instanced do
    sum += oscili(0.2, f)
  od
  want:a = oscili(0.2, 220) + oscili(0.2, 330)
  assertSameSignal(sum, want, "first definition")
  assertNotSilent(want, "first definition")
endin

instr 2
  ok:i = compilestr({{
instr 3
  freqs:i[] fillarray 440, 550, 660
  sum:a = 0
  for f:i in freqs instanced do
    sum += vco2(0.1, f)
  od
  want:a = vco2(0.1, 440) + vco2(0.1, 550) + vco2(0.1, 660)
  assertSameSignal(sum, want, "compiled later")
  assertNotSilent(want, "compiled later")
endin
}})
  if ok != 0 then
    prints("compilestr failed\n")
    exitnow(-1)
  endif
  schedule(3, 0, 0.3)
  schedule(1, 0, 0.3)
endin
</CsInstruments>
<CsScore>
i 1 0 0.3
i 2 0.4 0
e 1
</CsScore>
</CsoundSynthesizer>
