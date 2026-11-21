<CsTest>
description = "a dynamic UDO result must satisfy the sole fixed-rank overload at init"

[expect]
exit = "nonzero"
stderr = ["array rank mismatch: expected 2, got 1"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
opcode Forward(values:i[*]):i[*]
  xout values
endop
opcode MatrixOnly(values:i[][]):i
  xout lenarray(values, 0)
endop
instr 1
  vector:i[] fillarray 1, 2
  result:i = MatrixOnly(Forward(vector))
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
