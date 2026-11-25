<CsTest>
description = "sum rejects a matrix instead of using its first row"

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
  matrix:i[][] init 2, 3
  result:i = sum(matrix)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
