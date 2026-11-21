<CsTest>
description = "a rank-generic UDO result must match the caller's destination"

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

instr 1
  matrix:i[][] init 2, 3
  vector:i[] = Forward(matrix)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
