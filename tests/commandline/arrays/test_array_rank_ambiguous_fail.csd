<CsTest>
description = "unknown rank cannot choose between two fixed rank UDO overloads"

[expect]
exit = "nonzero"
stderr = ["ambiguous array rank overload"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#include "../libassert.orc"

opcode RankChoice(values:i[]):i
  choice:i = 1
  xout choice
endop
opcode RankChoice(values:i[][]):i
  choice:i = 2
  xout choice
endop
opcode Forward(values:i[*]):i[*]
  xout values
endop
instr 1
  matrix:i[][] init 2, 3
  result:i = RankChoice(Forward(matrix))
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
