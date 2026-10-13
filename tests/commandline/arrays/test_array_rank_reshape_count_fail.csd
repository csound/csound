<CsTest>
description = "reshape rejects a new shape with a different element count"

[expect]
exit = "nonzero"
stderr = ["reshapearray: the number of items does not match"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  vector:i[] init 6
  result:i[][] reshapearray vector, 2, 4
endin
</CsInstruments>
<CsScore>
i 1 0 0
</CsScore>
</CsoundSynthesizer>
