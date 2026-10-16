<CsTest>
description = "for-loop element type mismatch is rejected during syntax checking"

[expect]
exit = "nonzero"
stderr = ["for-loop variable 'S0' has type 'S', incompatible with array element type 'i'"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d --syntax-check-only
</CsOptions>
<CsInstruments>
instr 1
  iarr[] fillarray 1, 2, 3
  for S0 in iarr do
  od
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
</CsScore>
</CsoundSynthesizer>
