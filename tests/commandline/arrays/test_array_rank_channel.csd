<CsTest>
description = "array channel rank follows each value written to it"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#include "../libassert.orc"

instr 1
  vector:i[] fillarray 11, 22
  matrix:i[][] init 2, 3
  matrix[1][2] = 42
  chnset vector, "changing-rank"
  first:i[] chnget "changing-rank"
  assertEquals(first[1], 22)
  chnset matrix, "changing-rank"
  second:i[][] chnget "changing-rank"
  assertEquals(second[1][2], 42)
  chnset vector, "changing-rank"
  third:i[] chnget "changing-rank"
  assertEquals(third[1], 22)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
