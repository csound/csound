<CsTest>
description = "instanced loop over an empty array runs no iterations"

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
  freqs:i[] init 0
  count:k = 0
  for f:i, n:i in freqs instanced do
    count += 1
  od
  if count != 0 then
    printks("empty array: ran %d iterations\n", 0, count)
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
