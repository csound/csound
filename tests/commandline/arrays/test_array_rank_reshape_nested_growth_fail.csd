<CsTest>
description = "reshape reports nested array growth that needs performance allocation"

[expect]
exit = "nonzero"
stderr = ["reshapearray: could not copy struct element into prepared storage"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
struct Nested values:k[]

instr 1
  kValues[] init 2
  kLong[] init 3
  nested:Nested init kValues
  source:Nested[] init 1
  source[0] = nested
  matrix:Nested[][] reshapearray source, 1, 1
  ; Reshape has already reserved the original nested array capacity.
  source[0].values = kLong
endin
</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
