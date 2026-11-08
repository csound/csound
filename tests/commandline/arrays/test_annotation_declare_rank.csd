<CsTest>
description = "declare and UDO parameters accept compact and spaced array annotations"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#include "../libassert.orc"

; These signatures must work before the UDO definitions have been read.
declare CompactCell(values:i[][]):i
declare SpacedCell(values:i[] []):i

instr 1
  matrix:i[][] init 2, 3
  matrix[1][2] = 42
  assertEquals(CompactCell(matrix), 42)
  assertEquals(SpacedCell(matrix), 42)
endin

opcode CompactCell(values:i[][]):i
  xout values[1][2]
endop

opcode SpacedCell(values:i [] []):i
  xout values[1][2]
endop
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
