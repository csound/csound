<CsTest>
description = "a fixed-rank UDO checks a generic function argument before binding it"

[expect]
exit = "nonzero"
stderr = ["rank mismatch", "expected 1, got 2"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
opcode Forward(values:i[*]):i[*]
  xout values
endop

opcode VectorSize(values:i[]):i
  xout lenarray(values)
endop

instr 1
  matrix:i[][] init 2, 3
  result:i = VectorSize(Forward(matrix))
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
