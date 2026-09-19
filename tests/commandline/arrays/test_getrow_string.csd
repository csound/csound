<CsTest>
description = "getrow and setrow preserve every string in a row"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-ndm0
</CsOptions>
<CsInstruments>
#include "../libassert.orc"

instr 1
  ; Use the second row so both row offsets and element offsets matter.
  src:S[][] init 2, 3
  src fillarray "a", "b", "c", "d", "e", "f"
  row:S[] getrow src, 1
  assert(strcmp(row[0], "d") == 0, "getrow: wrong first value\n")
  assert(strcmp(row[1], "e") == 0, "getrow: wrong middle value\n")
  assert(strcmp(row[2], "f") == 0, "getrow: wrong last value\n")
  turnoff
endin

instr 2
  row:S[] fillarray "X", "Y", "Z"
  out:S[][] setrow row, 1
  assert(strcmp(out[1][0], "X") == 0, "setrow: wrong first value\n")
  assert(strcmp(out[1][1], "Y") == 0, "setrow: wrong middle value\n")
  assert(strcmp(out[1][2], "Z") == 0, "setrow: wrong last value\n")
  turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 2 0 0.01
</CsScore>
</CsoundSynthesizer>
