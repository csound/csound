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
giDestNonPower ftgen 4, 0, 500, 7, 0, 500, 0
giSrcNonPower ftgen 5, 0, 300, 7, 10, 300, 20
giSrcNonPower2 ftgen 6, 0, 500, 7, 0, 500, 0

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
  turnoff
endin

instr 2
  tableimix 4, 0, 500, 5, 0, 1, 6, 0, 0
  assert(abs(table:i(300, 4) - 0.5) < 1e-6,
         "tableimix did not wrap the destination index\n")
  prints "tableimix non-power destination passed"
  exitnow 0
endin
</CsInstruments>
<CsScore>
i1 0 1
i2 0 1
</CsScore>
</CsoundSynthesizer>
