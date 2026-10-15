<CsTest>
description = "instanced loops reject arrays of strings"

[expect]
exit = "nonzero"
stderr = ["instanced loops require an array of numeric values"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
0dbfs = 1

instr 1
  for word:S in ["a", "b"] instanced do
    prints(word)
  od
endin
</CsInstruments>
<CsScore>
i 1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
