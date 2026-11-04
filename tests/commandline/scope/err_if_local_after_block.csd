<CsTest>
description = "A typed variable declared in a k-rate if body is not visible after endif"
[expect]
exit = "nonzero"
stderr = ["'x' is declared inside a block", "and is not visible outside it"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  if p4 == 1 then
    x:k = 7
  endif
  printks("x=%d\n", 1, x)
endin
</CsInstruments>
<CsScore>
i1 0 0.1 1
i1 0.2 0.1 0
</CsScore>
</CsoundSynthesizer>
