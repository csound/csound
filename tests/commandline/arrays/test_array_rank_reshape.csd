<CsTest>
description = "reshape copies between declared ranks and preserves its source"
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#include "../libassert.orc"

struct Nested values:i[]

instr 1
  vector:i[] fillarray 1, 2, 3, 4, 5, 6
  matrix:i[][] reshapearray vector, 2, 3
  assertEquals(lenarray(matrix, 0), 2)
  assertEquals(lenarray(matrix, 1), 2)
  assertEquals(lenarray(matrix, 2), 3)
  assertEquals(matrix[1][2], 6)
  assertEquals(lenarray(vector, 0), 1)
  assertEquals(lenarray(vector), 6)

  matrix[1][2] = 42
  assertEquals(vector[5], 6)
  flattened:i[] reshapearray matrix, 6
  assertEquals(flattened[5], 42)
  flattened[0] = 99
  assertEquals(matrix[0][0], 1)

  ; In-place changes may alter sizes while retaining the declared rank.
  reshapearray matrix, 3, 2
  assertEquals(lenarray(matrix, 1), 3)
  assertEquals(lenarray(matrix, 2), 2)
  assertEquals(matrix[2][1], 42)

  cube:i[][][] reshapearray vector, 1, 2, 3
  assertEquals(cube[0][1][2], 6)

  nested:Nested init vector
  source:Nested[] init 1
  source[0] = nested
  copied:Nested[][] reshapearray source, 1, 1
  assertEquals(copied[0][0].values[5], 6)
  replacement:i[] fillarray 10, 20, 30, 40, 50, 99
  copied[0][0].values = replacement
  assertEquals(source[0].values[5], 6)
  assertEquals(copied[0][0].values[5], 99)
endin
</CsInstruments>
<CsScore>
i 1 0 0
</CsScore>
</CsoundSynthesizer>
