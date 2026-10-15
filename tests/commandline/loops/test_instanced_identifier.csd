<CsTest>
description = "instanced is not a reserved word outside for loops"

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

#include "../libassert.orc"

instr 1
  instanced:i = 2
  total:i = 0
  for v:i in [1, 2, 3] do
    total += v * instanced
  od
  assertEquals(total, 12)
endin
</CsInstruments>
<CsScore>
i 1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
