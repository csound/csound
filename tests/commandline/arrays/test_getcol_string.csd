<CsoundSynthesizer>
<CsOptions>
-ndm0
</CsOptions>
<CsInstruments>
#include "../libassert.orc"

instr 1
  src:S[][] init 2, 3
  src fillarray "a", "b", "c", "d", "e", "f"
  col:S[] getcol src, 2
  assert(strcmp(col[0], "c") == 0, "getcol.S returned the wrong first value\n")
  assert(strcmp(col[1], "f") == 0, "getcol.S returned the wrong second value\n")
  turnoff
endin

instr 2
  col:S[] init 2
  col fillarray "X", "Y"
  out:S[][] setcol col, 2
  assert(strcmp(out[0][2], "X") == 0, "setcol.S wrote the wrong first value\n")
  assert(strcmp(out[1][2], "Y") == 0, "setcol.S wrote the wrong second value\n")
  turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 2 0 0.01
</CsScore>
</CsoundSynthesizer>
