<CsTest>
description = "A typed variable declared in a for body is not visible after the loop"
[expect]
exit = "nonzero"
stderr = ["'last' is declared inside a block", "and is not visible outside it"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  for k in [1, 2, 3] do
    if p4 == 1 then
      last:i = k
    endif
  od
  prints("last=%d\n", last)
endin
</CsInstruments>
<CsScore>
i1 0 0.1 1
i1 0.2 0.1 0
</CsScore>
</CsoundSynthesizer>
