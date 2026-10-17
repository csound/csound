<CsTest>
description = "elementwise expressions keep the rank needed for overload selection"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#include "../libassert.orc"

opcode RankChoice(values:i[]):i
  choice:i = 1
  xout choice
endop
opcode RankChoice(values:i[][]):i
  choice:i = 2
  xout choice
endop
instr 1
  vector:i[] fillarray 1, 2
  matrix:i[][] init 2, 3
  assertEquals(RankChoice(matrix * 2), 2)
  assertEquals(RankChoice(vector + 1), 1)
  assertEquals(RankChoice(getrow(matrix, 0)), 1)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
