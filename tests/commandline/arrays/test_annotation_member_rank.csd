<CsTest>
description = "struct members preserve every array dimension in type annotations"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#include "../libassert.orc"

struct Compact values:i[][]
struct Spaced values:i [][]
struct Split values:i[] []
struct Inferred iValues[][]

instr 1
  iSource[][] init 2, 3
  iSource[1][2] = 42

  compact:Compact init iSource
  spaced:Spaced init iSource
  split:Split init iSource
  inferred:Inferred init iSource

  ; All three spellings must describe the same member type.
  assert(strcmp(typeof(compact.values), "i[][]") == 0, "compact member rank\n")
  assert(strcmp(typeof(spaced.values), "i[][]") == 0, "spaced member rank\n")
  assert(strcmp(typeof(split.values), "i[][]") == 0, "split member rank\n")
  assert(strcmp(typeof(inferred.iValues), "i[][]") == 0, "inferred member rank\n")

  iCopy[][] = compact.values
  assertEquals(iCopy[1][2], 42)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
