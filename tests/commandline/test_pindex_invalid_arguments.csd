<CsTest>
description = "pindex rejects non-string results and out-of-range indices"

[expect]
exit = "nonzero"
stderr = ["pindex: p-field 4 is not a string", "invalid p field index"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
ksmps = 32
instr 1
  SValue = "existing result"
  SValue pindex 4
endin
instr 2
  iValue pindex 1e30
endin
instr 3
  SValue pindex 1e30
endin
</CsInstruments>
<CsScore>
i 1 0 .01 42
i 2 0 .01
i 3 0 .01
e
</CsScore>
</CsoundSynthesizer>
