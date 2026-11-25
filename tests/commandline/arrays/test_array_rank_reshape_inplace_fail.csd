<CsTest>
description = "in-place reshape rejects a rank change"

[expect]
exit = "nonzero"
stderr = ["reshapearray: cannot change array rank in place"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  vector:i[] init 6
  reshapearray vector, 2, 3
endin
</CsInstruments>
<CsScore>
i 1 0 0
</CsScore>
</CsoundSynthesizer>
