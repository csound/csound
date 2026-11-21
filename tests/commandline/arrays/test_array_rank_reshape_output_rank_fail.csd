<CsTest>
description = "reshape sizes must match the output array's declared rank"

[expect]
exit = "nonzero"
stderr = ["reshapearray: output array rank mismatch"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  vector:i[] init 6
  result:i[] reshapearray vector, 2, 3
endin
</CsInstruments>
<CsScore>
i 1 0 0
</CsScore>
</CsoundSynthesizer>
