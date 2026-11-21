<CsTest>
description = "reshape rejects a source size change during performance"

[expect]
exit = "nonzero"
stderr = ["reshapearray: input element count changed"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
instr 1
  kSource[] init 6
  kShort[] init 2
  kMatrix[][] reshapearray kSource, 2, 3
  kSource = kShort
endin
</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
