<CsTest>
description = "a UDO with a vector input rejects a matrix"

[expect]
exit = "nonzero"
stderr = ["array rank mismatch", "expected 1, got 2"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
opcode VectorSize(values:i[]):i
  xout lenarray(values)
endop

instr 1
  matrix:i[][] init 2, 3
  result:i = VectorSize(matrix)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
