<CsTest>
description = "an existing array cannot be redeclared at another rank"

[expect]
exit = "nonzero"
stderr = ["rank mismatch", "expected 1, got 2"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>

instr 1
  values:i[] init 2
  values:i[][] init 2, 3
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
