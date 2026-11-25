<CsTest>
description = "reshape rejects different input and output element types"

[expect]
exit = "nonzero"
stderr = ["reshapearray: array element types do not match"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  vector:i[] init 6
  result:S[][] reshapearray vector, 2, 3
endin
</CsInstruments>
<CsScore>
i 1 0 0
</CsScore>
</CsoundSynthesizer>
