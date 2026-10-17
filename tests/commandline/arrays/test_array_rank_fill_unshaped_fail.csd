<CsTest>
description = "fillarray rejects a matrix without initialized sizes"

[expect]
exit = "nonzero"
stderr = ["fillarray: initialize matrix sizes before filling it"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  values:i[][] fillarray 1, 2, 3, 4, 5, 6
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
