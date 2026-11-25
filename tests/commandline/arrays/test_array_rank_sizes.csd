<CsTest>
description = "array sizes may change while the declared rank stays fixed"

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
  iSource[][] init 4, 5
  iSource[3][4] = 42
  iTarget[][] init 2, 3
  iTarget = iSource
  assertEquals(lenarray(iTarget, 1), 4)
  assertEquals(lenarray(iTarget, 2), 5)
  assertEquals(iTarget[3][4], 42)

  ; A function result must keep both dimensions when used in an expression.
  copied:i[][] = init:i[][](2, 3)
  assertEquals(lenarray(copied, 0), 2)
  assertEquals(lenarray(copied, 2), 3)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
