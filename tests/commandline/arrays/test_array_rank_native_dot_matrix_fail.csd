<CsTest>
description = "dot rejects a matrix as its second input"

[expect]
exit = "nonzero"
stderr = ["INIT ERROR", "one-dimensional"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  vector:i[] fillarray 1, 2
  matrix:i[][] init 2, 3
  result:i = dot(vector, matrix)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
