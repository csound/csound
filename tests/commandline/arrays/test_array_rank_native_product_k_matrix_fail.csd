<CsTest>
description = "product rejects a control matrix during performance"

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
  result:k = product(matrix)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
