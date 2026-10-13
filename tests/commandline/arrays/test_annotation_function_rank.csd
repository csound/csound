<CsTest>
description = "function calls accept multidimensional output annotations"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#include "../libassert.orc"

opcode LastCell(values:i[][]):i
  xout values[1][2]
endop

instr 1
  matrix:i[][] = init:i[][](2, 3)
  matrix[1][2] = 42
  assertEquals(LastCell(matrix), 42)

  cube:i[][][] = init:i[][][](2, 3, 4)
  assertEquals(lenarray(cube, 1), 2)
  assertEquals(lenarray(cube, 2), 3)
  assertEquals(lenarray(cube, 3), 4)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
