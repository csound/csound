<CsTest>
description = "any-rank numeric parameter still rejects string arrays"

[expect]
exit = "nonzero"
stderr = ["Unable to find opcode entry"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#include "../libassert.orc"

opcode RankChoice(values:i[*]):i
  xout lenarray(values, 0)
endop
instr 1
  words:S[] fillarray "hello", "world"
  result:i = RankChoice(words)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
