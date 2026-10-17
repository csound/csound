<CsTest>
description = "rank-generic UDO inputs and outputs accept vectors and matrices"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#include "../libassert.orc"

struct Item value:i

opcode ArrayRank(values:i[*]):i
  xout lenarray(values, 0)
endop

opcode Forward(values:i[*]):i[*]
  xout values
endop

opcode ForwardItems(values:Item[*]):Item[*]
  xout values
endop

instr 1
  vector:i[] fillarray 1, 2, 3
  matrix:i[][] init 2, 3
  matrix[1][2] = 42
  assertEquals(ArrayRank(vector), 1)
  assertEquals(ArrayRank(matrix), 2)

  vectorCopy:i[] = Forward(vector)
  matrixCopy:i[][] = Forward(matrix)
  assertEquals(vectorCopy[2], 3)
  assertEquals(matrixCopy[1][2], 42)

  ; Explicit generic return annotations also allow spaces around the brackets.
  compact:i[][] = Forward:i[*](matrix)
  spaced:i[][] = Forward:i [*] (matrix)
  assertEquals(lenarray(compact, 0), 2)
  assertEquals(compact[1][2], 42)
  assertEquals(lenarray(spaced, 0), 2)
  assertEquals(spaced[1][2], 42)

  items:Item[][] init 2, 3
  last:Item init 84
  items[1][2] = last
  itemCopy:Item[][] = ForwardItems(items)
  assertEquals(itemCopy[1][2].value, 84)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
