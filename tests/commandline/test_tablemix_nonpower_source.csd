<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>
#include "libassert.orc"

sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

giDest ftgen 1, 0, 8, -2, 0, 0, 0, 0, 0, 0, 0, 0
giSrc1 ftgen 2, 0, 6, -2, 10, 20, 30, 40, 50, 60
giSrc2 ftgen 3, 0, 8, -2, 0, 0, 0, 0, 0, 0, 0, 0

instr 1
  tableimix 1, 0, 8, 2, 0, 1, 3, 0, 0
  assertEquals(table:i(0, 1), 10)
  assertEquals(table:i(1, 1), 20)
  assertEquals(table:i(2, 1), 30)
  assertEquals(table:i(3, 1), 40)
  assertEquals(table:i(4, 1), 50)
  assertEquals(table:i(5, 1), 60)
  assertEquals(table:i(6, 1), 10)
  assertEquals(table:i(7, 1), 20)
  prints "tableimix non-power source passed"
  exitnow 0
endin
</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
