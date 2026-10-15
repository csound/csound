<CsTest>
description = "known ranks prefer exact UDO overloads regardless of definition order"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#include "../libassert.orc"

; Put the generic overload first to catch first-match selection.
opcode RankChoice(values:i[*]):i
  choice:i = 0
  xout choice
endop
opcode RankChoice(values:i[][]):i
  choice:i = 2
  xout choice
endop
opcode Forward(values:i[*]):i[*]
  xout values
endop
; This pair uses the opposite definition order.
opcode ReverseChoice(values:i[][]):i
  choice:i = 2
  xout choice
endop
opcode ReverseChoice(values:i[*]):i
  choice:i = 0
  xout choice
endop
opcode MatrixOnly(values:i[][]):i
  xout lenarray(values, 0)
endop

instr 1
  vector:i[] fillarray 1, 2
  matrix:i[][] init 2, 3
  assertEquals(RankChoice(matrix), 2)
  assertEquals(RankChoice(vector), 0)
  assertEquals(ReverseChoice(matrix), 2)
  assertEquals(ReverseChoice(vector), 0)
  ; With only one fixed-rank overload, check the unknown rank at init.
  assertEquals(MatrixOnly(Forward(matrix)), 2)
  ; Forward does not promise a rank, so choose the generic overload.
  assertEquals(RankChoice(Forward(matrix)), 0)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
