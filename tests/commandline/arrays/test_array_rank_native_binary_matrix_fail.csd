<CsTest>
description = "binary rejects matrix inputs instead of truncating them"

[expect]
exit = "nonzero"
stderr = ["one-dimensional"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  matrix:i[][] init 2, 3
  vector:i[] fillarray 1, 2
  result:i[] = pow(vector, matrix)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
