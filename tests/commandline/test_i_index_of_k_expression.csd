<CsTest>
description = "i() of an array refuses an index that is a k-rate expression, which has no value at init"

[expect]
exit = "nonzero"
stderr = ["Use of i() with a k-rate expression as index not permitted"]
output = ["Parsing failed due to 7 semantic errors"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

; each index is computed in performance, so at init it would be 0 and each
; line would give values[0], 10, where values[2] is 30
instr 1
  index:k init 2
  offsets:k[] fillarray 1, 2
  places:i[] fillarray 1, 2
  values:k[] fillarray 10, 20, 30
  sum:i i values, index + 0
  called:i = i(values, abs(index))
  ternary:i = i(values, index > 0 ? index : 0)
  element:i = i(values, offsets[index - 1])
  ; an i array read at a k index is read in performance too
  place:i = i(values, places[index - 1] + 0)
  ; and so is a comparison of k values, which would be 0 rather than 1
  compared:i = i(values, index > 1)
  print sum, called, ternary, element, place, compared, i(values, index * 1)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
