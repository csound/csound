<CsTest>
description = "dot rejects a control matrix during performance"

[expect]
exit = "nonzero"
stderr = ["PERF ERROR", "one-dimensional"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  matrix:k[][] init 2, 3
  vector:k[] fillarray 1, 2
  result:k = dot(matrix, vector)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
