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

  ; Spaces between bracket pairs must not change the function's output type.
  spaced:i[][] = init:i[] [](2, 3)
  spaced[1][2] = 84
  assertEquals(LastCell(spaced), 84)
  assertEquals(lenarray(spaced, 1), 2)
  assertEquals(lenarray(spaced, 2), 3)

  ; Also allow space before the first pair and before the argument list.
  separated:i[][][] = init:i [] [] [] (2, 3, 4)
  assertEquals(lenarray(separated, 1), 2)
  assertEquals(lenarray(separated, 2), 3)
  assertEquals(lenarray(separated, 3), 4)

  ; Tabs are horizontal whitespace too.
  tabbed:i[][] = init:i[]	[](2, 3)
  assertEquals(lenarray(tabbed, 1), 2)
  assertEquals(lenarray(tabbed, 2), 3)

  ; A nested call keeps the same type, and opcode syntax still works.
  assertEquals(lenarray(fillarray:i [] (1, 2, 3)), 3)
  byOpcode:i[][] init (2), (3)
  assertEquals(lenarray(byOpcode, 1), 2)
  assertEquals(lenarray(byOpcode, 2), 3)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
