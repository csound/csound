<CsTest>
description = "fillarray keeps an initialized matrix's rank and sizes"

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
  values:i[][] init 2, 3
  values fillarray 1, 2, 3, 4, 5, 6
  assertEquals(lenarray(values, 0), 2)
  assertEquals(lenarray(values, 1), 2)
  assertEquals(lenarray(values, 2), 3)
  assertEquals(values[0][2], 3)
  assertEquals(values[1][0], 4)
  assertEquals(values[1][2], 6)

  words:S[][] init 2, 3
  words fillarray "a", "b", "c", "d", "e", "f"
  assertEquals(lenarray(words, 0), 2)
  assertEquals(lenarray(words, 1), 2)
  assertEquals(lenarray(words, 2), 3)
  assert(strcmp(words[0][2], "c") == 0, "first row changed\n")
  assert(strcmp(words[1][0], "d") == 0, "second row changed\n")
  assert(strcmp(words[1][2], "f") == 0, "last cell changed\n")
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
